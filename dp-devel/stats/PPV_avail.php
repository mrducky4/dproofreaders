<?
$relPath="../pinc/";
include_once($relPath.'dp_main.inc');
include_once($relPath.'project_states.inc');
include_once($relPath.'theme.inc');


$order = (isset($_GET['order']) ? $_GET['order'] : 'nameofwork' );
if ( $order == 'nameofwork' )
{
	$orderclause = 'nameofwork ASC';
}
elseif ( $order == 'modifieddate' )
{
	$orderclause = 'modifieddate ASC';
}
elseif ( $order == 'PPer' )
{
	$orderclause = 'postproofer ASC';
}
else
{
	echo "PPV_avail.php: bad order value: '$order'";
	exit;
}

// ------------------

theme("Books Available for PPV", "header");

echo "<h2>Books Available for PPV</h2>\n";

// ------------------

// Header row

$colspecs = array(
	'#'                  => 'bogus',
	'Name of Work'       => 'nameofwork',
	'Post-Processed By'  => 'postproofer',
	'Date Last Modified' => 'modifieddate'
);

echo "<table border='1'>\n";
echo "<tr>\n";
foreach ( $colspecs as $col_header => $col_order )
{
	$s = $col_header;
	// Make each column-header a link that will sort on that column,
	// except for the header of the column that we're already sorting on.
	if ( $col_order != $order && $col_order != 'bogus' )
	{
		$s = "<a href='checkedout.php?state=$state&order=$col_order'>$s</a>";
	}
	$s = "<th>$s</th>";
	echo "$s\n";
}
echo "</tr>\n";

// ------------------

// Body

$result = mysql_query("
	SELECT
		nameofwork,
		postproofer,
		modifieddate
	FROM projects
	WHERE state = '".PROJ_POST_SECOND_AVAILABLE."'
	ORDER BY $orderclause
");

$rownum = 0;
while ( $project = mysql_fetch_object( $result ) )
{
	$rownum++;

	//calc last modified date for project
	$today = getdate($project->modifieddate);
	$month = $today['month'];
	$mday = $today['mday'];
	$year = $today['year'];
	$datestamp = "$month $mday, $year";

	echo "
		<tr>
		<td>$rownum</td>
		<td width='200'>$project->nameofwork</td>
		<td>$project->postproofer</td>
		<td>$datestamp</td>
		</tr>
	";
}

echo "</table>";
theme("","footer");
?>
