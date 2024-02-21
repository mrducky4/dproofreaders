<?php

// Searching for book records in an external catalog
// via Z39.50 protocol (implemented by yaz library).

$relPath = '../../pinc/';
include_once($relPath.'base.inc');
include_once($relPath.'theme.inc');
include_once($relPath.'MARCRecord.inc');

require_login();

$action = @$_REQUEST['action'];

if ($action == 'show_query_form') {
    show_query_form();
} elseif ($action == "do_search_and_show_hits") {
    do_search_and_show_hits();
} else {
    die("unrecognized value for 'action' parameter: '$action'");
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function show_query_form()
{
    $title = _("Create a Project");
    output_header($title);

    if (!function_exists('yaz_connect')) {
        echo "<p class='error'>";
        echo _("PHP is not compiled with YAZ support.  Please do so and try again.");
        echo "</p>";
        echo "<p>";
        echo sprintf(
            _("Until you do so, click <a href='%s'>here</a> for creating a new project."),
            'editproject.php?action=createnew'
        );
        echo "</p>";
        echo "<p>";
        echo sprintf(
            _("If you believe you should be seeing the Create Project page please contact a <a href='%s'>Site Administrator</a>"),
            "mailto:".$GLOBALS['site_manager_email_addr']
        );
        echo "</p>";
    } else {
        echo "<h1>$title</h1>";

        echo "<p>";
        echo _("Please put in as much information as possible to search for your project.  The more information the better but if not accurate enough may rule out results.");
        echo "</p>";

        echo "<form method='post' action='external_catalog_search.php'>\n";
        echo "<input type='hidden' name='action' value='do_search_and_show_hits'>\n";
        echo "<table class='basic'>";

        foreach (
            [
                'title' => _('Title'),
                'author' => _('Author'),
                'publisher' => _('Publisher'),
                'pubdate' => _('Publication Year (eg: 1912)'),
                'isbn' => _('ISBN'),
                'issn' => _('ISSN'),
                'lccn' => _('LCCN'),
            ] as $field_name => $field_label
        ) {
            echo "<tr>";
            echo   "<th class='label'>$field_label</th>";
            echo   "<td>";
            echo     "<input type='text' size='30' name='$field_name' maxlength='255'>";
            echo   "</td>";
            echo "</tr>\n";
        }
        echo "<tr><th colspan='2'>";
        echo "<input type='checkbox' checked name='hide_nontext'>";
        echo _("Hide non-textual results");
        echo "</th></tr>";

        echo "<tr>";
        echo   "<th colspan='2'>";
        echo     "<input type='submit' value='", attr_safe(_('Search')), "'>";
        echo   "</th>";
        echo "</tr>\n";

        echo "</table>";
        echo "</form>\n";
    }
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function do_search_and_show_hits()
{
    output_header("Search Results");
    echo "<br>";
    $start = get_integer_param($_GET, 'start', 1, 1, null);
    $hide_nontext = !empty(@$_REQUEST['hide_nontext']);
    if (!empty($_GET['fq'])) {
        $fullquery = unserialize(base64_decode($_GET['fq']));
    } else {
        $fullquery = query_format();
    }

    global $external_catalog_locator;
    // We request UTF-8 character set, but according to the docs (and our testing)
    // most servers ignore this and return ISO-8859-1 anyway. The strings get
    // converted to UTF-8 via MARCRecord::__get() instead.
    $id = yaz_connect($external_catalog_locator, ["charset" => "UTF-8"]);
    yaz_syntax($id, "usmarc");
    yaz_element($id, "F");
    yaz_search($id, "rpn", trim(str_replace("\n", " ", $fullquery)));
    $extra_options = ["timeout" => 60];
    yaz_wait($extra_options);
    $errorMsg = yaz_error($id);

    echo "<details>\n";
    echo "<summary>Raw query</summary>\n";
    echo "<pre>$fullquery</pre>\n";
    echo "</details>\n";

    if (!empty($errorMsg)) {
        echo "<p class='error'>", _("The following error has occurred:"), " $errorMsg", "</p>";
        echo "<p>";
        $url = "editproject.php?action=createnew";
        echo sprintf(
            _("Please try again. If the problem recurs, please create your project manually by following this <a href='%s'>link</a>."),
            $url
        );
        echo "</p>";
        exit();
    }

    if (yaz_hits($id) == 0) {
        echo "<p class='warning'>", _("There were no results returned."), "</p>\n";
        echo "<p>", sprintf(_("Please search again or click '%s' to create the project manually."), _("No Matches")), "</p>\n";
    }

    // Read and parse results in a batch
    $total_hits = yaz_hits($id);
    $num_nontext = 0;
    $hits_per_page = 20; // Perhaps later this can be a PM preference or an option on the form.

    $marc_records = [];
    for ($i = 0; $i < $hits_per_page && $start + $i <= $total_hits; $i++) {
        $rec = yaz_record($id, $start + $i, "array");
        // if $rec isn't an array, then yaz_record() failed and we should
        // skip this record
        if (!is_array($rec)) { /** @phpstan-ignore-line */
            continue;
        }
        /** @phpstan-ignore-next-line */
        $marc_record = new MARCRecord();
        $marc_record->load_yaz_array($rec);
        if (!in_array($marc_record->get_type_of_record(), ["Language material", "Manuscript language material"])) {
            $num_nontext++;
            if ($hide_nontext) {
                continue;
            }
        }
        $marc_records[] = [$rec, $marc_record];
    }
    yaz_close($id);

    // Display results
    echo "<p>", sprintf(_("%d results returned."), $total_hits), "</p>\n";

    if ($total_hits > 0) {
        $encoded_fullquery = base64_encode(serialize($fullquery));
        $url_base = "external_catalog_search.php?action=do_search_and_show_hits&fq=$encoded_fullquery";
        if ($hide_nontext) {
            $url_base .= "&hide_nontext=on";
        }

        // PHPStan up to at least 1.10.57 has a bug where it doesn't correctly detect
        // that $num_nontext is updated in the loop. We need to sprinkle several ignores
        // to get it to ignore this.
        // @phpstan-ignore-next-line
        $book_frag = ($hide_nontext && $num_nontext > 0) ? sprintf(_(" (%d non-textual hidden)"), $num_nontext) : "";

        if ($num_nontext < $hits_per_page) { /** @phpstan-ignore-line */
            echo "<p>", _("Please pick a result from below:"), "</p>\n";

            display_navbar($url_base, $start, $hits_per_page, $total_hits, $book_frag);

            // Display the results as a two column, multi-row table where each cell is
            // an input form radio button with a single subtable of label/value rows.
            echo "<form method='post' action='editproject.php'>\n";
            echo "<input type='hidden' name='action' value='create_from_marc_record'>\n";
            echo "<table style='width: 100%; border: 0;'>\n";

            $i = 1;
            foreach ($marc_records as [$r, $m]) { /** @phpstan-ignore-line */
                if ($i % 2 == 1) {
                    echo "<tr>";
                }
                // Radio button to select record
                echo "<td class='center-align top-align' style='width: 5%;'>";
                echo "<input type='radio' name='rec' value='".base64_encode(serialize($r))."'>";
                echo "</td>";

                // Subtable for record
                echo "<td class='left-align top-align' style='width: 45%;'>";
                display_record_table($m);
                echo "<p>"; // vertical gap between records
                echo "</td>";
                if ($i % 2 != 1) {
                    echo "</tr>\n";
                }
                $i++;
            }

            if ($i % 2 != 1) { /** @phpstan-ignore-line */
                echo "</tr>\n";
            }
            echo "</table>";
        }
        // Always display the bottom navbar, even if all results on a page are filtered away
        display_navbar($url_base, $start, $hits_per_page, $total_hits, $book_frag);
    }

    // Button bar
    echo "<p class='center-align'>";
    if ($total_hits != 0) {
        echo "<input type='submit' value='", attr_safe(_("Create the Project")), "'>";
    }

    foreach ([
        [_('Search Again'), "external_catalog_search.php?action=show_query_form"],
        [_('No Matches'), "editproject.php?action=createnew"],
        [_('Quit'), "projectmgr.php"],
    ] as [$label, $url]) {
        echo "&nbsp;";
        echo "<input type='button' value='", attr_safe($label), "' onclick='javascript:location.href=\"$url\";'>";
    }
    echo "</p>";
    echo "</form>";
}

function link_or_text(string $url_base, string $label, bool $show_link, int $start): string
{
    return $show_link ? "<a href='$url_base&start=$start'>" . html_safe($label) . "</a>" : $label;
}

function display_navbar(string $url_base, int $start, int $hits_per_page, int $total_hits, string $book_frag): void
{
    // NB yaz indexes are 1-based: from 1 to $total_hits inclusive.
    // To spell out the arithmetic here (because it's easy to make off-by-one errors)
    // I'll use examples of start=51, hits_per_page=10, total_hits=100
    $next = $start + $hits_per_page;            // next page will be 61 to 70
    $prev = max(1, $start - $hits_per_page);    // prev page will be 41 to 50. If we start at <11, prev=1
    $last = $total_hits - ($hits_per_page - 1); // last page will be 91 to 100.
    $frags = [
        link_or_text($url_base, "First", $start > 1, 1),
        link_or_text($url_base, "Previous", $start > 1, $prev),
        sprintf(_("Results %d to %d of %d%s"), $start, min($total_hits, $next - 1), $total_hits, $book_frag),
        link_or_text($url_base, "Next", $next <= $total_hits, min($total_hits, $next)),
        link_or_text($url_base, "Last", $next <= $total_hits, $last),
    ];
    echo "<p class='center-align'>", implode(" | ", $frags), "</p>\n";
}

function display_record_table(MARCRecord $marc_record): void
{
    echo "<table class='basic' style='width: 100%;'>";
    foreach ([
        [_("Type of Record"), $marc_record->type_of_record],
        [_("Title"),          $marc_record->title],
        [_("Author"),         $marc_record->author],
        [_("Publisher"),      $marc_record->publisher],
        [_("Language"),       $marc_record->language],
        [_("LCCN"),           $marc_record->lccn],
        [_("ISBN"),           $marc_record->isbn],
    ] as [$label, $value]) {
        echo "<tr>";
        echo   "<th class='left-align top-align' style='width: 20%;'>{$label}:</th>";
        echo   "<td class='left-align top-align'>{$value}</td>";
        echo "</tr>\n";
    }
    echo "</table>";
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function query_format()
{
    // Build a Z39.50 Type-1 query.
    // See
    // https://www.loc.gov/z3950/agency/markup/09.html Type-1 and Type-101 Queries
    //
    //
    // A Type-1 query is in prefix polish notation and a (prettified) simple query
    // for editions of Snowcrash published >= 1992 might look like
    // ```
    //  @and
    //  @and
    //  @attr 1=4 "Snowcrash"
    //  @attr 1=1003 "Stephenson, Neal"
    //  @attr 2=4 @attr 1=31 1992
    // ```
    // Expressions like `@attr 1=1003` mean value 1003 from attribute set 1
    // Set 1 is "Bib-1 Use", and 1003 is the value "Author"
    // https://www.loc.gov/z3950/agency/defns/bib1.html
    // https://www.loc.gov/z3950/agency/bib1.html

    $fullquery = [];

    if ($_REQUEST['title']) {
        // Bib-1 Use, Title
        $fullquery[] = sprintf('@attr 1=4 "%s"', $_REQUEST['title']);
    }
    if ($_REQUEST['author']) {
        // Convert author to "Surname, Forename" if it doesn't already contain a comma.
        $author = $_REQUEST['author'];
        if (!str_contains($author, ",")) {
            $p = strrpos($author, " ");
            if ($p !== false) {
                $author = substr($author, $p) . ", " . substr($author, 0, $p);
            }
        }
        // Bib-1 Use, Author
        $fullquery[] = sprintf('@attr 1=1003 "%s"', trim($author));
    }
    if ($_REQUEST['isbn']) {
        // Bib-1 Relation Equal; Bib-1 Use, ISBN
        $fullquery[] = sprintf('@attr 2=3 @attr 1=7 %s', str_replace("-", "", $_REQUEST['isbn']));
    }
    if ($_REQUEST['issn']) {
        // Bib-1 Relation Equal; Bib-1 Use, ISSN
        $fullquery[] = sprintf('@attr 2=3 @attr 1=8 %s', $_REQUEST['issn']);
    }
    if ($_REQUEST['lccn']) {
        // Bib-1 Relation Equal; Bib-1 Use, LC call number
        $fullquery[] = sprintf('@attr 2=3 @attr 1=9 %s', $_REQUEST['lccn']);
    }
    if ($_REQUEST['pubdate']) {
        // Bib-1 Relation Equal; Bib-1 Use, Date of publication
        $fullquery[] = sprintf('@attr 2=3 @attr 1=31 "%s"', $_REQUEST['pubdate']);
    }
    if ($_REQUEST['publisher']) {
        // Bib-1 Use, Publisher
        $fullquery[] = sprintf('@attr 1=1018 "%s"', $_REQUEST['publisher']);
    }
    $c = count($fullquery) - 1;
    for ($i = 0; $i < $c; $i++) {
        array_unshift($fullquery, "@and");
    }
    return implode("\n", $fullquery);
}
