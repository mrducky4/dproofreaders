<?php
$relPath = "./../../pinc/";
include_once($relPath.'base.inc');
include_once($relPath.'theme.inc');
include_once($relPath.'Project.inc');
include_once($relPath.'wordcheck_engine.inc');
include_once('./post_files.inc');
include_once('./word_freq_table.inc');

require_login();

set_time_limit(0); // no time limit

$projectid = get_projectID_param($_REQUEST, 'projectid');
$freqCutoff = get_integer_param($_REQUEST, 'freqCutoff', 5, 0, null);

enforce_edit_authorization($projectid);

// $format determins what is presented from this page:
//   'html' - page is rendered with frequencies included
//   'file' - all words and frequencies are presented as a
//            downloaded file
// 'update' - update the list
$format = get_enumerated_param($_REQUEST, 'format', 'html', ['html', 'file', 'update']);

if ($format == "update") {
    $postedWords = parse_posted_words($_POST);

    $words = load_project_bad_words($projectid);
    $words = array_merge($words, $postedWords);
    save_project_bad_words($projectid, $words);

    $format = "html";
}

[$bad_words_w_freq, $messages] = _get_word_list($projectid);
$title = _("Candidates for Bad Words List from Site's Possible bad words file");
$page_text = _("Displayed below are the words from this project that are found in the site's Possible Bad Words files.");
$page_text .= " ";
$page_text = _("The results list was generated by accessing the most recent text of each page and comparing it to the site bad word suggestions file for each of the project's languages and excluding words already on the project's Bad Words List. The results list also shows how many times each word occurs in the project text.");

$page_text2 = _("Words in the Possible Bad Words file are frequently-encountered stealth scannos, and therefore they would not be flagged by the external spell check. The existence of these words in this project does not mean these words are stealth scannos, just that they might be.");

if ($format == "file") {
    $filename = "${projectid}_possible_bad_words.txt";
    header("Content-type: text/plain");
    header('Content-Disposition: attachment; filename="' . $filename . '"');
    // The cache-control and pragma is a hack for IE not accepting filenames
    header('Cache-Control: must-revalidate, post-check=0, pre-check=0');
    header('Pragma: public');

    echo $title . "\r\n";
    echo sprintf(_("Project: %s"), get_project_name($projectid)) . "\r\n";
    echo "\r\n";
    echo $page_text . "\r\n";
    echo $page_text2 . "\r\n";
    echo "\r\n";
    echo_page_instruction_text("bad", $format);
    echo "\r\n";
    echo_download_text($projectid, $format);
    echo "\r\n";
    echo _("Format: [word] - [frequency in text]") . "\r\n";
    echo "\r\n";

    foreach ($bad_words_w_freq as $word => $freq) {
        echo "$word - $freq\r\n";
    }

    // we're done here, exit
    exit;
} elseif ($format == "html") {
    // fall-through
} else {
    throw new UnexpectedValueException("Unexpected format $format");
}

// how many instances (ie: frequency sections) are there?
$instances = 1;
// what are the cutoff options?
$cutoffOptions = [1, 2, 3, 4, 5, 10, 25, 50];

output_header($title, NO_STATSBAR, ["js_data" => get_cutoff_script($cutoffOptions, $instances)]);

echo_page_header($title, $projectid);

// what is the intial cutoff frequecny?
$initialFreq = getInitialCutoff($freqCutoff, $cutoffOptions, $bad_words_w_freq);

echo "<p>$page_text</p>";

echo "<p>$page_text2</p>";

echo_page_instruction_text("bad", $format);

echo_download_text($projectid, $format);

echo_any_warnings_errors($messages);

echo_cutoff_text($initialFreq, $cutoffOptions);

$context_array = build_context_array_links($bad_words_w_freq, $projectid);
$context_array["[[TITLE]]"] = _("Show Context");

$word_checkbox = build_checkbox_array($bad_words_w_freq);

$checkbox_form["projectid"] = $projectid;
$checkbox_form["freqCutoff"] = $freqCutoff;
echo_checkbox_selects(count($bad_words_w_freq));
echo_checkbox_form_start($checkbox_form);
echo_checkbox_form_submit(_("Add selected words to Bad Words List"));

printTableFrequencies($initialFreq, $cutoffOptions, $bad_words_w_freq, $instances--, $context_array, $word_checkbox);

echo_checkbox_form_submit(_("Add selected words to Bad Words List"));
echo_checkbox_form_end();


//---------------------------------------------------------------------------
// supporting page functions

function _get_word_list($projectid)
{
    $messages = [];

    // get the latest project text of all pages up to last possible round
    $last_possible_round = get_Round_for_round_number(MAX_NUM_PAGE_EDITING_ROUNDS);
    $pages_res = page_info_query($projectid, $last_possible_round->id, 'LE');
    $all_words_w_freq = get_distinct_words_in_text(get_page_texts($pages_res));

    // load site word lists for project languages
    $site_possible_bad_words = load_site_possible_bad_words_given_project($projectid);

    // now, remove any words that are already on the project's bad word list
    $site_possible_bad_words = array_diff($site_possible_bad_words, load_project_bad_words($projectid));

    // $site_possible_bad_words doesn't have frequency info,
    // so start with the info in $all_words_w_freq,
    // and extract the items where the key matches a key in $bad_words.
    $bad_words_w_freq = array_intersect_key($all_words_w_freq, array_flip($site_possible_bad_words));

    // multisort screws up all-numeric words so we need to preprocess first
    prep_numeric_keys_for_multisort($bad_words_w_freq);

    // sort the list by frequency, then by word
    array_multisort(array_values($bad_words_w_freq), SORT_DESC, array_map('strtolower', array_keys($bad_words_w_freq)), SORT_ASC, $bad_words_w_freq);

    return [$bad_words_w_freq, $messages];
}
