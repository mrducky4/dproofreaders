<?php

include_once($relPath.'TallyBoard.inc');
include_once($relPath.'Settings.inc');
include_once($relPath.'User.inc');

// $ACCESS_CRITERIA are extended as activities are defined.

global $ACCESS_CRITERIA;
$ACCESS_CRITERIA = [
    'days since reg' => _('Days since registration'),
    'quiz/p_basic' => sprintf(_('Up-to-date completion of the <a href="%1$s">Basic Proofreading Quiz</a>'), "$code_url/quiz/start.php?show_level=P_BASIC"),
    'quiz/p_mod1' => sprintf(_('Up-to-date completion of the <a href="%1$s">Moderate Proofreading Quiz, Part %2$d</a>'), "$code_url/quiz/start.php?show_level=P_MOD", 1),
    'quiz/p_mod2' => sprintf(_('Up-to-date completion of the <a href="%1$s">Moderate Proofreading Quiz, Part %2$d</a>'), "$code_url/quiz/start.php?show_level=P_MOD", 2),
    'quiz/f_only' => sprintf(_("Up-to-date completion of the <a href='%s'>Formatting Quiz</a>"), "$code_url/quiz/start.php?show_only=format"),
];

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

/*
 * A singleton container class for accessing Activity and inherited objects
 * (Stages, Rounds, Pools).
 *
 * The Activity constructor adds the constructed object to this class and
 * because Stage, Round, and Pool classes inherit from the Activity class,
 * those objects are accessible from this class too. For code readability,
 * those classes have accessor classes of their own (Stages, Rounds, Pools)
 * that derive from Activities and specify which objects to access.
 */
class Activities
{
    // `$_activities` contains a mapping from a specific type of Activity (e.g. an
    // Activity, a Stage, a Round or a Pool) to the instances of those. They are mapped
    // by their name, called IDs). The logic accounts for the class hierarchy:
    // Stage derives from Activity so any instance will be on both the Stage and
    // the Activity lists.
    //
    // This poses a fundamental issue with adding types to this logic:
    // the type of each list depends on its name.
    //
    // A simple solution would be to return the top-level class Activity (to match how
    // `$_activities` is typed) but that confuses (rightly) PHPStan and would require a
    // lot of new type checks whenever callers manipulate these classes.
    //
    // Thus for now, we don't return types for several getters that manipulate
    // `$_activities`. Over time, we will have to find a way to disentangle
    // the different lists to add proper types.
    /** @var array<string, array<string, Activity>> */
    private static array $_activities = [];

    // This maps the static container class names to the class types
    // they contain. This makes the functions in this base class return the
    // correct subset of items when called from the child classes.
    private static $container_to_class_mapping = [
        "Activities" => "Activity",
        "Rounds" => "Round",
        "Stages" => "Stage",
        "Pools" => "Pool",
    ];

    private static function _get_class_mapping(): string
    {
        return self::$container_to_class_mapping[static::class];
    }

    public static function add(Activity &$activity): void
    {
        foreach (array_merge([get_class($activity)], class_parents($activity)) as $class) {
            assert(!isset(self::$_activities[$class][$activity->id]), "Duplicate activity ID; IDs must be unique");
            self::$_activities[$class][$activity->id] = $activity;
        }
    }

    /**
     * The return of this function is untyped for now (see the comment above `$_activities` for why).
     */
    public static function get_all(): array
    {
        return self::$_activities[self::_get_class_mapping()];
    }

    /** @return string[] */
    public static function get_ids(): array
    {
        // get_all() will ensure they are the right object types
        return array_keys(self::get_all());
    }

    /**
     * The return of this function is untyped for now (see the comment above `$_activities` for why).
     *
     * @param int|string $value
     */
    protected static function _get(string $attribute, $value)
    {
        // fast-track lookup by ID
        if ($attribute == "id") {
            return self::$_activities[self::_get_class_mapping()][$value] ?? null;
        }

        // loop through all objects looking for a match
        foreach (self::get_all() as $activity) {
            if (!property_exists($activity, $attribute)) {
                continue;
            }
            if (is_array($activity->$attribute) && in_array($value, $activity->$attribute)) {
                return $activity;
            } elseif ($activity->$attribute == $value) {
                return $activity;
            }
        }
        return null;
    }

    /**
     * The return of this function is untyped for now (see the comment above `$_activities` for why).
     */
    public static function get_by_id(string $id)
    {
        return self::$_activities[self::_get_class_mapping()][$id] ?? null;
    }
}

class Activity
{
    public string $id;
    public string $name;
    /** @var array<string,int> */
    public array $access_minima;
    public string $after_satisfying_minima;
    public string $evaluation_criteria;
    public ?string $access_change_callback;

    /**
     * Activity constructor
     *
     * @param string $id
     *   A very short mnemonic identifier for the activity.
     *   (Should probably conform to the rules for a PHP variable name.)
     * @param string $name
     *   A gettext-translated name for the activity.
     * @param array $access_minima
     *   A (possibly empty) array of minimum requirements that a user must satisfy
     *   in order to be allowed to participate in this activity
     *   (barring special permission).
     * @param string $after_satisfying_minima
     *   After satisfying the above minima (if any), does the user have to do
     *   anything else to work in this activity?
     *   ```
     *   'IMMEDIATE'  Nope, they get immediate access.
     *   'REQ-AUTO'   They must ask for access, but it is auto-granted.
     *   'REQ-HUMAN'  They must ask for access, and it must be human-granted.
     *   'NOREQ'      They don't request access (or at least, we don't supply
     *                a link by which to request access). Instead, they just
     *                wait until they are approved.
     *   ```
     * @param string $evaluation_criteria
     *   A brief description of what the evaluation criteria are.
     * @param ?string $access_change_callback
     *   A callback function that will be called when a user's access for this
     *   round has changed. The function will be passed a User object representing
     *   the user with the access change in addition to the access change itself, eg:
     *   ```
     *   p1_access_change_callback($user, $change)
     *   ```
     *   where $change is one of `[ "grant", "revoke", "request", "deny_request_for" ]`
     */
    public function __construct(
        string $id,
        string $name,
        /** @var array<string,int> */
        array $access_minima,
        string $after_satisfying_minima,
        string $evaluation_criteria,
        ?string $access_change_callback
    ) {
        $this->id = $id;
        $this->name = $name;
        $this->access_minima = $access_minima;
        $this->after_satisfying_minima = $after_satisfying_minima;
        $this->evaluation_criteria = $evaluation_criteria;
        $this->access_change_callback = $access_change_callback;

        if (SiteConfig::get()->testing && !is_null($this->access_minima)) { /** @phpstan-ignore-line */
            // Relax minima.
            foreach ($this->access_minima as $criterion_code => $minimum) {
                if (str_starts_with($criterion_code, 'quiz/')) {
                    // skip
                } elseif ($criterion_code == 'days since reg') {
                    $this->access_minima[$criterion_code] = intval($minimum / 7);
                } else {
                    $this->access_minima[$criterion_code] = intval($minimum / 30);
                }
            }
        }

        // This is kludgey, since Activity shouldn't have to know about Round.
        // But the alternative is that Round know about $ACCESS_CRITERIA,
        // which seems worse.
        // TODO: We should probably have a function for declaring criteria.
        global $ACCESS_CRITERIA;
        if (is_a($this, 'Round')) {
            $ACCESS_CRITERIA[$this->id] =
                sprintf(_("'%s' pages completed"), $this->id);
        }

        Activities::add($this);
    }

    public function __toString(): string
    {
        return $this->id;
    }

    /**
     * Return a User Access object
     *
     * Return an object with the following properties:
     * - can_access:
     *         a boolean: TRUE iff the user can access this activity.
     * - minima_table:
     *         an array of arrays (4-tuples):
     *         ( $criterion_str, $minimum, $user_score, $satisfied )
     * - all_minima_satisfied:
     *         boolean
     * - request_status:
     *         enumerated type: sat-unneeded, sat-granted, sat-available, sat-requested,
     *                          sat-wait, sat-denied
     *                          unsat-granted, unsat-requested, unsat-ungranted,
     *                          unsat-denied
     * - evaluation_criteria:
     *         message to display about evaluation
     *
     * UNIMPLEMENTED:
     * If $n_pages_completed is non-null, use it as the number of pages
     * that the user has completed. Otherwise, consult the database.
     */
    // TODO(jchaffraix): Add a class for the UserAccess object for type soundness.
    public function user_access(?string $username, $n_pages_completed = null): object
    {
        if (is_null($username)) {
            $uao = new StdClass(); // user access object
            $uao->username = $username;
            $uao->can_access = false;
            return $uao;
        }

        // A user's score isn't going to change in a page load but many page
        // loads will request an activity's user_access object multiple times
        // so cache the scores to reduces DB hits.
        static $_user_score_cache = [];
        $cache_index = "$this->id:$username";
        if (!array_key_exists($cache_index, $_user_score_cache)) {
            $_user_score_cache[$cache_index] = get_user_scores($username, $this->access_minima);
        }
        $user_scores = $_user_score_cache[$cache_index];

        // -----------------------------------
        // recorded_access is the value recorded in the db for whether the user
        // can access this activity. It's read from the usersettings table.
        // If there is a relevant usersetting, it can take values 'yes', 'no'
        // or 'denied'.
        // No relevant usersetting has the same effect as 'no'.
        $userSettings = & Settings::get_Settings($username);
        $recorded_access = $userSettings->get_value("{$this->id}.access", "no");

        // -----------------------------------

        $uao = new StdClass(); // user access object

        $uao->username = $username;
        $uao->activity_id = $this->id;
        $uao->evaluation_criteria = $this->evaluation_criteria;
        // Considering the minima...
        global $ACCESS_CRITERIA;
        {
            $uao->minima_table = [];
            $uao->all_minima_satisfied = true;
            foreach ($this->access_minima as $criterion_code => $minimum) {
                $criterion_str = $ACCESS_CRITERIA[$criterion_code];
                $user_score = $user_scores[$criterion_code];

                $satisfied = ($user_score >= $minimum);
                $uao->minima_table[$criterion_code] = [$criterion_str, $minimum, $user_score, $satisfied];
                if (!$satisfied) {
                    $uao->all_minima_satisfied = false;
                }
            }
        }

        if ($recorded_access == 'denied') {
            // they are not allowed to access this activity or to request access
            $uao->can_access = false;
            if ($uao->all_minima_satisfied) {
                $uao->request_status = 'sat-denied';
            } else {
                $uao->request_status = 'unsat-denied';
            }
        } elseif ($uao->all_minima_satisfied) {
            // They've satisfied the requirements...
            switch ($this->after_satisfying_minima) {
                case 'IMMEDIATE':
                    // They get immediate access.
                    $uao->request_status = 'sat-unneeded';
                    $uao->can_access = true;
                    break;

                case 'REQ-AUTO':
                    // They must ask for access, but it is auto-granted.
                    if ($recorded_access == 'yes') {
                        // They already have access
                        $uao->request_status = 'sat-granted';
                        $uao->can_access = true;
                    } else {
                        // They haven't asked yet, but can
                        $uao->request_status = 'sat-available';
                        $uao->can_access = false;
                    }
                    break;

                case 'REQ-HUMAN':
                    // They must ask for access, and it must be human-granted.
                    if ($recorded_access == 'yes') {
                        // They already have access
                        $uao->request_status = 'sat-granted';
                        $uao->can_access = true;
                    } elseif ($recorded_access == 'requested') {
                        // They've asked, but it hasn't been granted yet
                        $uao->request_status = 'sat-requested';
                        $uao->can_access = false;
                    } else {
                        // They haven't asked yet, but can
                        $uao->request_status = 'sat-available';
                        $uao->can_access = false;
                    }
                    break;

                case 'NOREQ':
                    // They don't request access (or at least, we don't supply a link
                    // by which to request access). Instead, they just wait until
                    // they are approved.
                    if ($recorded_access == 'yes') {
                        // They already have access
                        $uao->request_status = 'sat-granted';
                        $uao->can_access = true;
                    } else {
                        // They are waiting
                        $uao->request_status = 'sat-wait';
                        $uao->can_access = false;
                    }
                    break;

                default:
                    throw new UnexpectedValueException("bad after_satisfying_minima value: '$this->after_satisfying_minima'");
            }
        } else {
            // They don't satisfy the requirements
            if ($recorded_access == 'yes') {
                // but they already have access anyway
                $uao->request_status = 'unsat-granted';
                $uao->can_access = true;
            } elseif ($recorded_access == 'requested') {
                // but they've requested access, but don't have it yet
                // This is unusual, but can happen.
                // E.g., at some point in the past, they satisfied the minima and
                // requested access, but now they no longer satisfy the minima
                // (quiz-pass could expire, page-tally could be reduced by clears).
                $uao->request_status = 'unsat-requested';
                $uao->can_access = false;
            } else {
                // and they can't ask for access
                $uao->request_status = 'unsat-ungranted';
                $uao->can_access = false;
            }
        }

        return $uao;
    }
}

// --------------------------------------------------------------------------

/**
  * @param array<string, int|string> $criteria
  * @return array<string, float>
  */
function get_user_scores(string $username, array $criteria): array
{
    if (!$criteria) {
        return [];
    }

    $user_obj = $username == User::current_username() ? User::load_current() : new User($username);

    $user_scores = [];
    foreach (array_keys($criteria) as $criterion_code) {
        $user_scores[$criterion_code] = get_user_score($user_obj, $criterion_code);
    }
    return $user_scores;
}

function get_user_score(User $user_obj, string $criterion_code): float
{
    if ($criterion_code == 'days since reg') {
        $user_score = round((time() - $user_obj->date_created) / 86400, 1);
    } elseif (str_starts_with($criterion_code, 'quiz/')) {
        $quiz_id = substr($criterion_code, 5);
        $quiz = get_Quiz_with_id($quiz_id);
        $user_score = $quiz->user_has_passed($user_obj->username) ? 1 : 0;
    } else {
        $tally_name = $criterion_code;
        $tallyboard = new TallyBoard($tally_name, 'U');
        $user_score = $tallyboard->get_current_tally($user_obj->u_id);
    }
    return $user_score;
}

// --------------------------------------------------------------------------

function show_user_access_object(object $uao, bool $will_autogrant = false): void
{
    global $code_url;

    echo "<a name='Entrance_Requirements'></a>";

    if (count($uao->minima_table) == 0) {
        echo "<p>" . _("There are no minimum requirements associated with this activity.") . "</p>";
    } else {
        echo "<h2>" . _('Entrance Requirements') . "</h2>\n";
        echo "<p>" . _('To access this round, you must satisfy the following requirements.') . "</p>\n";
        echo "<table class='basic'>\n";

        echo "<tr>";
        echo "<th>" . _('Criterion') . "</th>";
        echo "<th>" . _('Minimum')  . "</th>";
        echo "<th>" . _('You')      . "</th>";
        echo "</tr>\n";

        foreach ($uao->minima_table as $row) {
            [$criterion_str, $minimum, $user_score, $satisfied] = $row;
            echo "<tr>";
            echo "<td>$criterion_str</td>";
            echo "<td class='right-align'>$minimum</td>";
            td_w_bgcolor($user_score, $satisfied);
            echo "</tr>\n";
        }
        echo "</table>\n";

        echo "<p>";
        echo "<strong>" . _('Eligibility status') . ":</strong> ";
        if ($uao->all_minima_satisfied) {
            echo _('You satisfy the requirements.');
        } else {
            echo _('Some requirements are not yet satisfied.') . ' ';
            echo $uao->evaluation_criteria;
        }
        echo "</p>";
    }
    echo "\n";

    $request_status_string = "";
    switch ($uao->request_status) {
        case 'sat-unneeded':
            $request_status_string = _('So you are allowed to work in this activity.');
            break;

        case 'sat-granted':
            $request_status_string = _('You have received permission to work in this activity.');
            break;

        case 'sat-requested':
        case 'unsat-requested':
            $request_status_string = _('You have requested permission, but it has not been granted yet.');
            break;

        case 'sat-available':
            $request_status_string = $uao->evaluation_criteria;
            $request_status_string .= " " . sprintf(
                _('If you would like to work in this activity, <a href="%s">click here</a> to submit a request.'),
                "$code_url/tools/request_access.php?stage_id={$uao->activity_id}"
            );
            break;

        case 'sat-wait':
            $request_status_string = _('However, you must wait for approval to work in this activity.');
            $request_status_string .= " " . $uao->evaluation_criteria;
            // Users in this activity will monitor your work and will let you know once you have qualified for access
            break;

        case 'sat-denied':
            $request_status_string = _('However, you are not currently permitted to apply for access to this activity.');
            break;

        case 'unsat-granted':
            $request_status_string = _("However, you have been granted access to this activity.");
            break;

        case 'unsat-ungranted':
            $activity = Activities::get_by_id($uao->activity_id);

            if ($activity->after_satisfying_minima == 'REQ-AUTO' && $will_autogrant) {
                $request_status_string = _('After you have satisfied the requirements, you will be automatically granted access.');
            } elseif ($activity->after_satisfying_minima == 'REQ-AUTO' || $activity->after_satisfying_minima == 'REQ-HUMAN') {
                $request_status_string = _('After you have satisfied the requirements, a link will appear here, allowing you to request access.');
            } else {
                // $request_status_string=_('After you have satisfied the requirements, you will be evaluated for access.');
            }
            // Don't mention possibility of special permission.
            // $request_status_string.=_("However, you can be granted access to this activity. See XXX for details.");
            break;

        case 'unsat-denied':
            // leave the string empty, so nothing prints out
            break;

        default:
            throw new UnexpectedValueException("bad request_status '$uao->request_status'");
    }

    if (!empty($request_status_string)) {
        echo "<p>$request_status_string</p>\n";
    }
}

function grant_user_access_if_sat(object &$uao): void
{
    $activity = Activities::get_by_id($uao->activity_id);
    if ($uao->request_status == 'sat-available' &&
        $activity->after_satisfying_minima == 'REQ-AUTO') {
        $user = new User($uao->username);
        $user->grant_access($uao->activity_id, 'AUTO-GRANTED');

        // now we need to reconstruct the $uao to reflect this change
        $uao = $activity->user_access($uao->username);
    }
}


// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function show_user_access_chart(?string $username): void
{
    global $ACCESS_CRITERIA, $code_url;

    if (is_null($username)) {
        throw new UnexpectedValueException('show_user_access_chart: $username is null');
    }

    [$allow_grant, $allow_revoke] = user_can_modify_access_of($username);

    if ($allow_grant || $allow_revoke) {
        echo "<form method='post' action='$code_url/tools/modify_access.php'>\n";
        echo "<input type='hidden' name='subject_username' value='$username'>\n";
    }

    echo "<table class='basic'>\n";

    // top header line
    {
        $n_criteria = count($ACCESS_CRITERIA);
        echo "<tr>";
        echo "<th></th>";
        echo "<th colspan='$n_criteria' class='center-align'>" . _("Criteria") . "</th>";
        // TRANSLATORS: abbreviation for "all satisfied?"
        echo "<th rowspan='3'>" . _("all sat?") . "</th>";
        echo "<th rowspan='3'>" . _("request") . "</th>";
        if ($allow_revoke) {
            echo "<th rowspan='3'>" . _("deny request?") . "</th>";
        }
        echo "<th rowspan='3'>" . _("can access?") . "</th>";
        if ($allow_grant) {
            echo "<th rowspan='3'>" . _("grant?") . "</th>";
        }
        if ($allow_revoke) {
            echo "<th rowspan='3'>" . _("revoke?") . "</th>";
        }
        echo "</tr>\n";
    }

    // bottom header line
    {
        echo "<tr>";
        echo "<th></th>";
        foreach ($ACCESS_CRITERIA as $criterion_code => $criterion_descr) {
            echo "<th>$criterion_code</th>";
        }
        echo "</tr>\n";
    }

    // user scores line
    {
        $user_scores = get_user_scores($username, $ACCESS_CRITERIA);

        echo "<tr>";
        echo "<th>" . _("user score:") . "</th>";
        foreach ($ACCESS_CRITERIA as $criterion_code => $criterion_descr) {
            $user_score = $user_scores[$criterion_code];
            echo "<td class='right-align'>$user_score</td>";
        }
        echo "</tr>\n";
    }

    foreach (Activities::get_all() as $activity) {
        $uao = $activity->user_access($username);

        echo "<tr>";
        echo "<th>$activity->id</th>";

        // "Criteria" columns
        foreach ($ACCESS_CRITERIA as $criterion_code => $criterion_descr) {
            $row = @$uao->minima_table[$criterion_code];
            if (is_null($row)) {
                echo "<td></td>";
            } else {
                [$criterion_str, $minimum, $user_score, $satisfied] = $row;
                td_w_bgcolor($minimum, $satisfied);
            }
        }

        // "all sat?" column
        td_w_bgcolor(
            ($uao->all_minima_satisfied ? _('Yes') : _('No')),
            $uao->all_minima_satisfied
        );

        $could_grant = false;
        $could_revoke = false;
        $could_deny_request = false;

        // "request" column
        echo "<td>";
        switch ($uao->request_status) {
            case 'sat-unneeded':
                // TRANSLATORS: abbreviation for "unnecessary"
                echo _('unnec');
                break;

            case 'sat-granted':
            case 'unsat-granted':
                echo _('granted');
                $could_revoke = true;
                break;

            case 'sat-requested':
            case 'unsat-requested':
                echo _('requested');
                $could_grant = true;
                $could_deny_request = true;
                break;

            case 'sat-wait':
                echo _('waiting');
                $could_grant = true;
                break;

            case 'sat-available':
                echo _('not yet');
                $could_grant = true;
                break;

            case 'unsat-ungranted':
                $could_grant = true;
                break;

            case 'sat-denied':
            case 'unsat-denied':
                echo _('denied');
                break;


            default:
                throw new UnexpectedValueException("bad request_status '$uao->request_status'");
        }
        echo "</td>";

        if ($allow_revoke) {
            // "deny request?" column
            echo "<td>";
            if ($could_deny_request) {
                echo "<input type='checkbox' name='{$activity->id}|deny_request_for'>";
            }
            echo "</td>";
        }

        // "can access?" column
        td_w_bgcolor(($uao->can_access ? _('Yes') : _('No')), $uao->can_access);

        if ($allow_grant) {
            // "grant?" column
            echo "<td>";
            if ($could_grant) {
                echo "<input type='checkbox' name='{$activity->id}|grant'>";
            }
            echo "</td>";
        }

        if ($allow_revoke) {
            // "revoke?" column
            echo "<td>";
            if ($could_revoke) {
                echo "<input type='checkbox' name='{$activity->id}|revoke'>";
            }
            echo "</td>";
        }

        echo "</tr>\n";
    }

    echo "</table>\n";

    if ($allow_grant || $allow_revoke) {
        echo "<input type='checkbox' name='notify_user'>" . _("Notify user of change") . "<br>\n";
        echo "<input type='submit' value='". attr_safe(_("Submit Changes")) . "'>\n";
        echo "</form>\n";
    }
}

function td_w_bgcolor(string $text, bool $bool): void
{
    $cell_class = $bool ? 'satisfied' : 'not_satisfied';
    echo "<td class='$cell_class'>$text</td>";
}
