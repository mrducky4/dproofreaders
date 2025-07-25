<?php

// forum_interface_phpbb3.inc

// This file provides functions to interface with the phpBB3 software.
// Ideally if the forum software changes due to version upgrades or
// using an entirely different forum software, this is one of the
// few files that would require changes.
//
// Currently only phpBB versions 3.0, 3.1, 3.2, and 3.3

/*
Important notes about using phpBB code within DP
------------------------------------------------
The forum interface code goes through great lengths to keep the DP code
and the phpBB code separate, that is to not include phpBB code directly
in the DP code.

For example, to create a topic, we invoke call_phpbb_function(), which
spawns a separate PHP-CLI process to run phpbb3.inc,
which executes phpBB code to create the topic. The reason for all of this
effort is to reduce namespace and variable collisions between DP and phpBB.

However, when we run phpBB code in separate process, it does not have access
to the server (e.g., $_SESSION and $_COOKIE and setcookie). Therefore, there
are two places where we can't use the PHP-CLI approach: login and logout.
Both of these activities interact with browser cookies and in the case of
login, uses the requester's IP address. In these two instances we are forced
to pull in phpBB code directly into the running DP process. Callers of
login_forum_user() and logout_forum_user() should make every effort to reduce
the amount of DP code included in those pages.
*/

use Symfony\Component\Process\Process;

/**
 * Return a table name with the phpBB table prefix prepended to it
 */
function phpbb_table($table)
{
    return SiteConfig::get()->forums_phpbb_table_prefix . "_" . $table;
}

/**
 * Return the language string phpbb uses for $langcode for languages
 * installed in the forum interface.
 *
 * Fall back to English if $langcode is not installed on the system.
 */
function phpbb_lang($locale = false)
{
    if ($locale === false) {
        $locale = get_desired_language();
    }

    // We're given a locale (eg: fr_FR) but phpBB languages are different.
    // Some samples: en, en_us, de, es_x_tu, es-mx, pt_br, pt
    // We can only do so much, so try for an exact match in lowercase
    // first, fall back to just the two-letter language code, and then
    // just fall back to English (which phpBB comes with by default).
    $lang_attempts = [strtolower($locale), substr($locale, 0, 2)];
    foreach ($lang_attempts as $lang) {
        $query = sprintf(
            "
            SELECT *
            FROM %s
            WHERE lang_iso='%s'
            ",
            phpbb_table("lang"),
            DPDatabase::escape($lang)
        );
        $res = DPDatabase::query($query);

        if (mysqli_num_rows($res) == 1) {
            return $lang;
        }
    }
    return "en";
}

/**
 * Implement or call the hashing function used by the forum to hash
 * the password stored in the database.
 *
 * This is required because we store the hashed password in the
 * non_activated_users table and use it directly when creating the account
 * post-activation.
 */
function forum_password_hash($password)
{
    // phpBB3 hashing function is involved, so we'll call it directly
    // instead of duplicating it here, but we don't want to pollute
    // the DP codespace so we'll do it via phpbb3.inc.
    $args = func_get_args();
    return call_phpbb_function('encode_and_hash_password', $args);
}

/**
 * Creates a forum user with the specified username, password, and email.
 *
 * The DP registration process already digests the password, hence the
 * need for $password_is_digested.
 *
 * Returns:
 * - TRUE  - user successfully created
 * - !TRUE (error string) - user creation failed
 *
 * Note: Use a strict comparison operator (=== or !==) against TRUE
 * to evaluate the pass/fail status of this function.
 */
function create_forum_user($username, $password, $email, $password_is_digested = false)
{
    // Use the phpBB3 API to create the user, but do so via phpbb3.inc
    // as to not mix the DP and phpBB3 codespace.
    $args = [
        0 => $username,
        1 => $password,
        2 => $email,
        3 => phpbb_lang(),
    ];
    $user_id = call_phpbb_function('create_user', $args);

    // create_user() should return a numeric $user_id, if it doesn't
    // it likely returned a phpBB3 error string. Lets return that instead.
    if (!is_numeric($user_id)) {
        return $user_id;
    }

    // if $password_is_digested we have to update the password in the DB directly
    if ($password_is_digested) {
        $sql = sprintf(
            "
            UPDATE %s
            SET user_password='%s'
            WHERE user_id=%d
            ",
            phpbb_table("users"),
            DPDatabase::escape($password),
            $user_id
        );
        DPDatabase::query($sql);
    }

    return true;
}

function phpbb_user_error_handler($errno, $errstr, $errfile, $errline)
{
    if (in_array($errno, [E_USER_ERROR, E_USER_NOTICE])) {
        throw new ValueError($errstr);
    }

    return false;
}

/**
 * Log in the user to the forums.
 *
 * Returns:
 * - [TRUE, '']  - user successfully logged in
 * - [FALSE, $reason] - login failed and the reason why, valid $reasons:
 *   - incorrect_username
 *   - incorrect_password
 *   - too_many_attempts
 *   - unknown
 */
function login_forum_user($username, $password)
{
    $forums_phpbb_dir = SiteConfig::get()->forums_phpbb_dir;

    if (!is_dir($forums_phpbb_dir)) {
        return false;
    }

    // phpbb code sets variables (e.g. $db) upon include assuming they are
    // global and accesses them in functions via e.g. global $db. However,
    // when we include phpbb files within this dp function, those variables
    // will now be set at function-scope, and so global $db will no longer
    // access the intended variable. So we have to insert global
    // declarations for the phpbb3 variables, so that when the
    // variable-assignment occurs at function-scope, it affects the global
    // variable.

    global $phpbb_root_path, $phpEx;
    $phpbb_root_path = $forums_phpbb_dir."/";
    $phpEx = "php";

    // these globals are pulled from
    // phpBB2/includes/sessions.php:setup()
    global $db, $template, $config, $auth, $phpEx, $phpbb_root_path, $cache;
    global $SID, $_SID, $user;
    // required for phpBB 3.1
    global $request, $symfony_request, $phpbb_filesystem;
    // required for phpBB 3.2
    global $phpbb_container, $phpbb_dispatcher, $table_prefix;

    define('IN_PHPBB', true);
    define('ROOT_PATH', $forums_phpbb_dir);

    include($phpbb_root_path . 'common.' . $phpEx);

    // set mysqli logging to pre-PHP 8.1 defaults
    mysqli_report(MYSQLI_REPORT_OFF);

    $user->session_begin();
    $auth->acl($user->data);

    // Upon initial login, if the password is in the phpBB2 format
    // it will be converted to the new format. It does this by using
    // $_REQUEST['password'], so we need to populate that here.
    $_REQUEST['password'] = $password;

    // phpBB3's login code does pre-processing on the username and
    // password before passing them into $auth->login(). To ensure
    // that passwords accepted by phpBB3 work when passed in here,
    // we have to do similar pre-processing.
    set_var($username, $username, 'string', true);
    set_var($password, $password, 'string', true);

    $persist_login = true;

    // phpBB may use trigger_error() on login if the user is banned which
    // doesn't return, so we need to capture that and handle it here
    $old_error_handler = set_error_handler("phpbb_user_error_handler");

    try {
        $results = $auth->login($username, $password, $persist_login);
        $status = $results["status"];
    } catch (ValueError $exception) {
        // Unfortunately, the error message might be in a language that isn't
        // English, so we need to check for a few possible combinations.
        // Thankfully the " ban" prefix covers many languages.
        if (
            str_contains($exception->getMessage(), " ban") ||      // en, fr, it, pt
            str_contains($exception->getMessage(), "gesperrt") ||  // de
            str_contains($exception->getMessage(), "exclusión") || // es
            str_contains($exception->getMessage(), "verbannen")    // nl
        ) {
            $status = "BANNED";
        } else {
            $status = "UNKNOWN";
        }
    }

    set_error_handler($old_error_handler);

    $success = $status == LOGIN_SUCCESS;
    switch ($status) {
        case LOGIN_SUCCESS:
            $reason = '';
            break;
        case LOGIN_ERROR_USERNAME:
            $reason = 'incorrect_username';
            break;
        case LOGIN_ERROR_PASSWORD:
            $reason = 'incorrect_password';
            break;
        case LOGIN_ERROR_ATTEMPTS:
            $reason = 'too_many_attempts';
            break;
        case "BANNED":
            $reason = 'banned';
            break;
        default:
            $reason = 'unknown';
            break;
    }

    // reset mysqli logging
    mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

    return [$success, $reason];
}

/**
 * Log out the currently-authenticated user from the forum.
 */
function logout_forum_user()
{
    global $pguser;

    $forums_phpbb_dir = SiteConfig::get()->forums_phpbb_dir;
    if (!is_dir($forums_phpbb_dir)) {
        return;
    }

    // See login_forum_user() for about the need for the global
    // declarations below.

    global $phpbb_root_path, $phpEx;
    $phpbb_root_path = $forums_phpbb_dir."/";
    $phpEx = "php";

    // these globals are pulled from
    // phpBB2/includes/sessions.php:session_create()
    global $SID, $_SID, $db, $config, $cache, $auth;
    global $user;
    // required for phpBB 3.1
    global $request, $symfony_request, $phpbb_filesystem;
    // required for phpBB 3.2
    global $phpbb_container, $phpbb_dispatcher, $table_prefix;

    define('IN_PHPBB', true);
    define('ROOT_PATH', $forums_phpbb_dir);

    include($phpbb_root_path . 'common.' . $phpEx);

    // set mysqli logging to pre-PHP 8.1 defaults
    mysqli_report(MYSQLI_REPORT_OFF);

    // Re-initialize phpBB session object in $user so we can kill it
    $user->session_begin();
    $user->session_kill();

    // reset mysqli logging
    mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);
}

function get_reset_password_url()
{
    return get_url_for_forum() . "/ucp.php?mode=sendpassword";
}

/**
 * Given a username, return details about the user.
 *
 * Returns an associative array. See $interested_columns for the keys.
 * If the user isn't found, the function returns NULL.
 */
function get_forum_user_details($username)
{
    $interested_columns = [
        "id",         // forum user id
        "username",   // forum username
        "lastvisit",  // timestamp the user last visited
        "from",       // "location" in phpBB 3.1
        "occ",        // "occupation" in phpBB 3.1
        "interests",  // interests
        "viewemail",  // if their email address is viewable
        "email",      // email
        "avatar",     // filename of their avatar, if defined
        "icq",        // ICQ address
        "website",    // website URL
        "aim",        // AIM address ("aol" in phpBB 3.1)
        "yim",        // Yahoo address ("yahoo" in 3.1)
        "msnm",       // MS Messenger address
        "jabber",     // Jabber address
        "rank",       // forum rank (index)
        "posts",      // number of forum posts
        // phpBB 3.1 and later return these keys as well
        "location",   // location (same as from)
        "occupation", // occupation (same as occ)
        "yahoo",      // Yahoo address (same as yim)
        "aol",        // AIM address (same as aim)
        "facebook",   // Facebook username
        "googleplus", // Google+ username
        "skype",      // Skype username
        "twitter",    // Twitter username
        "youtube",    // YouTube username
        "avatar_type",   // what type of avatar is this? (upload, gravatar, remote)
        "avatar_width",  // width of avatar
        "avatar_height", // height of avatar
    ];

    $return_data = [];
    $query = sprintf(
        "
        SELECT *
        FROM %s
        WHERE username='%s'
        ",
        phpbb_table("users"),
        DPDatabase::escape($username)
    );
    $res = DPDatabase::query($query);

    $row = mysqli_fetch_assoc($res);
    if (!$row) {
        return null;
    }

    foreach ($interested_columns as $column) {
        $return_data[$column] = $row["user_$column"] ?? ($row[$column] ?? null);
    }

    // phpBB 3.1 and later put data in phpbb_profile_fields_data with
    // slightly different column names. We will return data for both the
    // old and new column names.
    $column_mapping = [
        // new => old
        "occupation" => "occ",
        "yahoo" => "yim",
        "aol" => "aim",
        "location" => "from",
    ];
    $query = sprintf(
        "
        SELECT *
        FROM %s
        WHERE user_id = %d
        ",
        phpbb_table("profile_fields_data"),
        $row["user_id"]
    );

    // This will fail on phpBB < 3.1, which is fine and expected
    $res = DPDatabase::query($query, false);
    if ($res && $row = mysqli_fetch_assoc($res)) {
        foreach ($row as $column => $value) {
            $column_suffix = str_replace("pf_phpbb_", "", $column);
            if (isset($column_mapping[$column_suffix])) {
                $return_data[$column_mapping[$column_suffix]] = $value;
            }
            $return_data[$column_suffix] = $value;
        }
    }

    return $return_data;
}

/**
 * Given a forum username, return the forum user ID.
 */
function get_forum_user_id($username)
{
    // Use a local in-memory cache so we don't pummel the database
    // for pages that end up calling this for the same small set
    // of users over and over. Cache is local to this function.
    static $uidCache = [];
    // if it's in the cache, return it
    if (isset($uidCache[$username])) {
        return $uidCache[$username];
    }
    $query = sprintf(
        "
        SELECT user_id
        FROM %s
        WHERE username = '%s'
        ",
        phpbb_table("users"),
        DPDatabase::escape($username)
    );
    $res = DPDatabase::query($query);

    $row = mysqli_fetch_assoc($res);
    mysqli_free_result($res);

    if (!$row) {
        return null;
    }

    // update the cache
    $uidCache[$username] = $row["user_id"];

    return $row["user_id"];
}

/**
 * Given a forum rank number, return the text title of that rank.
 */
function get_forum_rank_title($rank)
{
    $query = sprintf(
        "
        SELECT rank_title
        FROM %s
        WHERE rank_id = %d
        ",
        phpbb_table("ranks"),
        $rank
    );
    $res = DPDatabase::query($query);

    $row = mysqli_fetch_assoc($res);
    mysqli_free_result($res);

    if (!$row) {
        return null;
    }

    return $row["rank_title"];
}

/**
 * Given a forum username, return the email address.
 */
function get_forum_email_address($username)
{
    $user_details = get_forum_user_details($username);

    return $user_details["email"] ?? "";
}

/**
 * Return a URL to the base of the forum
 */
function get_url_for_forum()
{
    return SiteConfig::get()->forums_phpbb_url;
}

/**
 * Return a URL that a user can use to manually log in to the forums.
 */
function get_url_for_forum_user_login()
{
    return get_url_for_forum() . "/ucp.php?mode=login";
}

/**
 * Given a forum username, return a URL that can be used to
 * access a form to send the user a message.
 */
function get_url_to_compose_message_to_user($username)
{
    $userid = get_forum_user_id($username);

    return get_url_for_forum() . "/ucp.php?i=pm&mode=compose&u=$userid";
}

/**
 * Given a forum id, return a URL that can be used to view the forum.
 */
function get_url_to_view_forum(int $forum_id): string
{
    return get_url_for_forum() . "/viewforum.php?f=$forum_id";
}

/**
 * Given a topic id, return a URL that can be used to view the topic.
 */
function get_url_to_view_topic($topic_id)
{
    return get_url_for_forum() . "/viewtopic.php?t=$topic_id";
}

/**
 * Given a post id, return a URL that can be used to view the post.
 */
function get_url_to_view_post($post_id)
{
    return get_url_for_forum() . "/viewtopic.php?p=$post_id#$post_id";
}

/**
 * Given a forum username, return a URL that can be used to load the user's
 * avatar.
 *
 * If no avatar is defined, this function returns NULL.
 */
function get_url_for_user_avatar($username)
{
    $user_details = get_forum_user_details($username);

    if (empty($user_details["avatar"])) {
        return null;
    }

    switch ($user_details["avatar_type"]) {
        case "avatar.driver.upload":
            $url = get_url_for_forum() . "/download/file.php?avatar=" . $user_details["avatar"];
            break;
        case "avatar.driver.remote":
            $url = $user_details["avatar"];
            break;
        case "avatar.driver.gravatar":
            $url = "https://www.gravatar.com/avatar/" .
                    md5(strtolower(trim($user_details["avatar"]))) .
                    "?s=" . $user_details["avatar_height"];
            break;
        default:
            $url = null;
    }
    return $url;
}

/**
 * Return a URL that can be used to edit the current user's profile.
 */
function get_url_to_edit_profile()
{
    return get_url_for_forum() . "/ucp.php";
}

/**
 * Return a URL that can be used to view a given user's profile.
 */
function get_url_to_view_profile($user_id)
{
    return get_url_for_forum() . "/memberlist.php?mode=viewprofile&u=$user_id";
}

/**
 * Return the URL for accessing the current user's inbox.
 */
function get_url_for_inbox()
{
    return get_url_for_forum() . "/ucp.php?i=pm&folder=inbox";
}

/**
 * Return the URL for searching the forums
 */
function get_url_for_search(string $query, bool $encode = true)
{
    return get_url_for_forum() . "/search.php?keywords=" . ($encode ? urlencode($query) : $query);
}

/**
 * Given a forum username, return the number of unread messages.
 */
function get_number_of_unread_messages($username)
{
    $forum_userid = get_forum_user_id($username);

    if ($forum_userid == null) {
        return null;
    }

    // from: includes/functions_privmsgs.php:get_folder($user_id)
    $query = sprintf(
        "
        SELECT SUM(pm_unread)
        FROM %s
        WHERE user_id = %d
        ",
        phpbb_table("privmsgs_to"),
        $forum_userid
    );

    $res = DPDatabase::query($query);

    [$num_messages] = mysqli_fetch_row($res);
    mysqli_free_result($res);

    return $num_messages;
}

/**
 * Confirm a specific topic ID exists.
 *
 * @return bool
 */
function does_topic_exist($topic_id)
{
    $query = sprintf(
        "
        SELECT 1
        FROM %s
        WHERE topic_id = %d
        ",
        phpbb_table("topics"),
        $topic_id
    );
    $res = DPDatabase::query($query);

    $exists = (mysqli_num_rows($res) > 0);

    mysqli_free_result($res);

    return $exists;
}

/**
 * Given a forum topic, return the time of the last post
 * in UNIX time format (seconds since UNIX epoch).
 *
 * If no topic is found, function returns NULL.
 */
function get_last_post_time_in_topic($topic_id)
{
    // Validate that $topic_id is an integer and if not, return NULL
    if (!is_numeric($topic_id)) {
        return null;
    }
    $query = sprintf(
        "
        SELECT MAX(post_time) as max_post_time
        FROM %s
        WHERE topic_id = %d
        ",
        phpbb_table("posts"),
        $topic_id
    );
    $res = DPDatabase::query($query);

    $row = mysqli_fetch_assoc($res);
    mysqli_free_result($res);

    if (!$row) {
        return null;
    }

    return $row["max_post_time"];
}

/**
 * Return details about a topic
 *
 * Returns the following details about a topic as an associative array.
 * - topic_id         - the ID of the topic (for completeness)
 * - title            - the title of the topic
 * - num_replies      - the number of replies in the topic
 * - forum_name       - name of the forum the topic is in
 * - forum_id         - the id of the forum the topic is in
 * - creator_username - the username of the topic creator
 */
function get_topic_details($topic_id)
{
    $forums_phpbb_dir = SiteConfig::get()->forums_phpbb_dir;

    $phpbb_topics = phpbb_table("topics");
    $phpbb_forums = phpbb_table("forums");
    $phpbb_users = phpbb_table("users");

    $query = "
        SELECT $phpbb_topics.topic_id AS topic_id,
            $phpbb_topics.topic_title AS title,
            IF($phpbb_topics.topic_posts_approved > 0, $phpbb_topics.topic_posts_approved - 1, 0) AS num_replies,
            $phpbb_forums.forum_name AS forum_name,
            $phpbb_forums.forum_id AS forum_id,
            $phpbb_users.username AS creator_username
        FROM $phpbb_topics
        INNER JOIN $phpbb_forums ON $phpbb_topics.forum_id = $phpbb_forums.forum_id
        INNER JOIN $phpbb_users ON $phpbb_topics.topic_poster = $phpbb_users.user_id
        WHERE $phpbb_topics.topic_id = " . sprintf("%d", $topic_id);
    $res = DPDatabase::query($query);

    $row = mysqli_fetch_assoc($res);
    mysqli_free_result($res);

    return $row;
}

// ---------------------------------------------------------------------------

// functions from the original pinc/topic.inc

// Parameters for topic_create() and topic_add_post()
// are in the order Where, What, Who:
//     Where: $forum_id or $topic_id
//     What:  $post_subject, $post_text
//     Who:   $poster_name, $poster_is_real

// -----------------------------------------------------------------------------

/**
 * Create a topic
 *
 * A proxy for phpbb3_create_topic() in external phpbb3.inc
 */
function topic_create(
    $forum_id,
    $post_subject,
    $post_text,
    $poster_name,
    $poster_is_real,
    $make_poster_watch_topic
): ?int {
    $args = func_get_args();
    $topic_id = call_phpbb_function('create_topic', $args);

    if (!preg_match('/^\d+$/', $topic_id)) {
        $topic_id = null;
    } else {
        $topic_id = (int)$topic_id;
    }

    return $topic_id;
}

// -----------------------------------------------------------------------------

/**
 * Create a post
 *
 * A proxy for phpbb3_add_post() in external phpbb3.inc
 */
function topic_add_post(
    $topic_id,
    $post_subject,
    $post_text,
    $poster_name,
    $poster_is_real
) {
    $args = func_get_args();
    call_phpbb_function('add_post', $args);
}

// -----------------------------------------------------------------------------

/**
 * Call a phpBB function in a separate process.
 *
 * It's risky to combine DP and phpBB code in the same PHP context,
 * so we run the phpBB code in a separate process.
 */
function call_phpbb_function($func_name, $args)
{
    // If $DEBUG is TRUE, output some additional debugging information useful
    // for figuring out where the calls into phpbb*.inc went wrong.
    $DEBUG = false;

    if ($DEBUG) { /** @phpstan-ignore-line */
        echo "call_phpbb_function( $func_name, ", print_r($args, true), ")<br>\n";
    }

    $forums_phpbb_dir = SiteConfig::get()->forums_phpbb_dir;
    if (!is_dir($forums_phpbb_dir)) {
        echo "Warning: unable to call '$func_name' because \$forums_phpbb_dir ($forums_phpbb_dir) does not exist.\n";
        return;
    }

    // The phpbb3.inc assumes it's running from $code_dir/pinc/ so we need
    // to run it from there specifically.
    global $code_dir;
    $php_cli_executable = SiteConfig::get()->php_cli_executable;
    $phpbb_include_file = "phpbb3.inc";
    $process = new Process(array_merge([$php_cli_executable, "-f", $phpbb_include_file, $func_name], $args), "$code_dir/pinc");
    $exitcode = $process->run();
    $output = $process->getOutput();
    $output_lines = explode("\n", rtrim($output));
    $last_line = end($output_lines);
    if (!$process->isSuccessful() || $DEBUG) { /** @phpstan-ignore-line */
        echo "<pre>\n";
        echo "$func_name command exited with status=$exitcode\n";
        echo "Here's its output:\n";
        echo $output;
        exit;
    }

    return $last_line;
}

// -----------------------------------------------------------------------------

function topic_change_forum($topic_id, $to_forum_id)
{
    assert($topic_id != '');

    call_phpbb_function('move_topic', [$topic_id, $to_forum_id]);
}

// -----------------------------------------------------------------------------

function forum_resynch($forum_id)
{
    call_phpbb_function('sync_forum', [$forum_id]);
}
