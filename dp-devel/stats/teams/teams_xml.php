<?
$relPath="./../../pinc/";
include_once($relPath.'v_site.inc');
include_once($relPath.'connect.inc');
include_once('../includes/team.php');
include_once('../includes/member.php');
$db_Connection=new dbConnect();

function xmlencode($data) {
	$trans_array = array();
	for ($i=127; $i<255; $i++) {
		$trans_array[chr($i)] = "&#" . $i . ";";
		}
      	$data = strtr($data, $trans_array);
	$data = htmlentities($data, ENT_QUOTES);
       	return $data;
}

if (empty($_GET['id']) || $_GET['id'] == 1) {
	include_once($relPath.'theme.inc');
	theme("Error!", "header");
	echo "<br><center>A team id must specified in the following format:<br>$code_url/stats/teams/teams_xml.php?id=****</center>";
	theme("", "footer");
	exit();
}

//Try our best to make sure no browser caches the page
header("Content-Type: text/xml");
header("Expires: Sat, 1 Jan 2000 05:00:00 GMT");
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . "GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");

$result = mysql_query("SELECT * FROM user_teams WHERE id = ".$_GET['id']."");
$curTeam = mysql_fetch_assoc($result);

//Team info portion of $data
	$result = mysql_query("SELECT id FROM user_teams WHERE id != 1 ORDER BY page_count DESC");
	$i = 1;
	while ($row = mysql_fetch_assoc($result)) {
        	if ($row['id'] == $curTeam['id']) { $pageCountRank = $i; break; }
		$i++;
	}
	$result = mysql_query("SELECT COUNT(id) AS totalTeams FROM user_teams");
	$totalTeams = (mysql_result($result, 0, "totalTeams") - 1);

	$result = mysql_query("SELECT date_updated, daily_page_count FROM user_teams_stats WHERE team_id = ".$curTeam['id']." ORDER BY daily_page_count DESC LIMIT 1");
	$bestDayCount = mysql_result($result, 0, "daily_page_count");
	$bestDayTime = date("M. jS, Y", (mysql_result($result, 0, "date_updated")-86400));

	$data = "<teaminfo id=\"".$curTeam['id']."\">
			<teamname>".xmlencode($curTeam['teamname'])."</teamname>
			<datecreated>".date("m/d/Y", $curTeam['created'])."</datecreated>
			<leader>".xmlencode($curTeam['createdby'])."</leader>
			<totalpages>".$curTeam['page_count']."</totalpages>
			<description>".xmlencode($curTeam['team_info'])."</description>
			<website>".xmlencode($curTeam['webpage'])."</website>
			<forums>".xmlencode($GLOBALS['forums_url']."/viewtopic.php?t=".$curTeam['topic_id'])."</forums>
			<totalmembers>".$curTeam['member_count']."</totalmembers>
			<currentmembers>".$curTeam['active_members']."</currentmembers>
			<retiredmembers>".($curTeam['member_count'] - $curTeam['active_members'])."</retiredmembers>
			<rank>".$pageCountRank."/".$totalTeams."</rank>
			<avgpagesday>".$curTeam['daily_average']."</avgpagesday>
			<mostpagesday>".$bestDayCount." (".$bestDayTime.")</mostpagesday>
		</teaminfo>
	";

//Team members portion of $data
	$data .= "<teammembers>";
	$mbrQuery = mysql_query("SELECT username, pagescompleted, date_created, u_id FROM users WHERE team_1 = ".$curTeam['id']." || team_2 = ".$curTeam['id']." || team_3 = ".$curTeam['id']." ORDER BY username ASC");
	while ($curMbr = mysql_fetch_assoc($mbrQuery)) {
			$data .= "<member id=\"".$curMbr['u_id']."\">
				<username>".xmlencode($curMbr['username'])."</username>
				<pagescompleted>".$curMbr['pagescompleted']."</pagescompleted>
				<datejoined>".date("m/d/Y", $curMbr['date_created'])."</datejoined>
			</member>
			";
	}
	$data .= "</teammembers>";


$xmlpage = "<"."?"."xml version=\"1.0\" encoding=\"ISO-8859-1\" ?".">
<teamstats xmlns:xsi=\"http://www.w3.org/2000/10/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"teamstats.xsd\">
$data
</teamstats>";

echo $xmlpage;
?>