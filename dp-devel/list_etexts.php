<?php
$relPath="./pinc/";
include($relPath.'connect.inc');
include($relPath.'bookpages.inc');
$db_Connection=new dbConnect();
include_once($relPath.'project_states.inc');

if($_GET['x'] == "g" OR $_GET['x'] == "") {
$type = "Gold";
$status = "Completed";
$state = "state=79";
$info = "Below is the list of Gold e-texts that have passed through this site.  Gold e-texts are books that have passed through a first round proofing, second round proofing and then a post processing.  After that they are then submitted to the Project Gutenberg database for your enjoyment and download.  These e-texts are the product of hundreds of hours of labor donated by all of our volunteers.  The list is sorted with the most recently submitted e-texts at the top.  You can sort them based upon your own preferences by clicking below.  Enjoy!!";
} elseif ($_GET['x'] == "s") {
$type = "Silver";
$status = "In Progress";
$state = "state>=40 AND state<=78";
$info = "Below is the list of Silver e-texts that are almost finished with their proofreading life on our site.  Silver e-texts are books that have passed through a first round proofing, second round proofing and are now in the post-processing phase.  During the post-processing phase the project manager runs the e-text through some final checks to make sure they are as correct as possible.  After that they are then submitted to the Project Gutenberg database for your enjoyment and download.  These e-texts are the product of hundreds of hours of labor donated by all of our volunteers.  The list is sorted with the most recently submitted e-texts at the top.  You can sort them based upon your own preferences by clicking below.  Enjoy!!";
} elseif ($_GET['x'] == "b") {
$type = "Bronze";
$status = "Now Proofing";
$state = "state='".AVAIL_PI_FIRST."' OR state='".AVAIL_PI_SECOND."' OR state='".VERIFY_PI_FIRST."' OR state='".VERIFY_PI_SECOND."'";
$info = "Below is the list of Bronze e-texts that are currently available for proofreading on this site.  Bronze e-texts is what most of our members see and what you can work on now by logging in.  These e-texts are either in the first round or second round proofing where you have a chance to correct any mistakes that may be found.  After the proofing phase the e-text is then passed onto the Project Manager for post-processing.  After that they are then submitted to the Project Gutenberg database for your enjoyment and download.  These e-texts are the product of hundreds of hours of labor donated by all of our volunteers.  The list is sorted with the most recently submitted e-texts at the top.  You can sort them based upon your own preferences by clicking below.  Enjoy!!";
}
?>

<html>
<head>
<title><? echo $status." ".$type; ?> E-Texts</title>
</head>

<body alink="#0000FF" vlink="#0000ff" link="#0000ff">
<center><font face="Verdana" size="6" color="<? echo $type; ?>"><b><? echo $status." ".$type; ?> E-Texts</b></font></center>
<center>
<?
if ($type == "Gold") {
echo "<a href='list_etexts.php?x=s'>Silver</a> | <a href='list_etexts.php?x=b'>Bronze</a>";
} elseif ($type == "Silver") {
echo "<a href='list_etexts.php?x=g'>Gold</a> | <a href='list_etexts.php?x=b'>Bronze</a>";
} elseif ($type == "Bronze") {
echo "<a href='list_etexts.php?x=g'>Gold</a> | <a href='list_etexts.php?x=s'>Silver</a>";
}
?>
 | <a href="default.php">Home</a></center>
<br>

<center><? echo $info; ?></center><br>

<center>
<i>Title:</i> <a href="list_etexts.php?x=<? echo $_GET['x']; ?>&sort=0">asc</a> or <a href="list_etexts.php?x=<? echo $_GET['x']; ?>&amp;sort=1">desc</a> | 
<i>Author:</i> <a href="list_etexts.php?x=<? echo $_GET['x']; ?>&sort=2">asc</a> or <a href="list_etexts.php?x=<? echo $_GET['x']; ?>&amp;sort=3">desc</a> | 
<i>Submitted Date:</i> <a href="list_etexts.php?x=<? echo $_GET['x']; ?>&sort=4">asc</a> or <a href="list_etexts.php?x=<? echo $_GET['x']; ?>&amp;sort=5">desc</a><br></center>
<hr width="75%" align="center"><br>

<?
$sort = isset($_GET['sort'])? $_GET['sort'] : 0;

$sortlist[0]=" Order by nameofwork asc";
$sortlist[1]=" Order by nameofwork desc";
$sortlist[2]=" Order by authorsname asc";
$sortlist[3]=" Order by authorsname desc";
$sortlist[4]=" Order by modifieddate asc";
$sortlist[5]=" Order by modifieddate desc";

$result=mysql_query("SELECT nameofwork, authorsname, ziplink, txtlink, htmllink, modifieddate, postednum, projectid FROM projects WHERE $state $sortlist[$sort]");

$numofetexts = 1;
while($row = mysql_fetch_array($result)) {

if ($type == "Gold") {
$links="";
if (trim($row['ziplink']) <> "") $links=$links."<a href='".$row['ziplink']."'>zip version</a>, ";
if (trim($row['txtlink']) <> "") $links=$links."<a href='".$row['txtlink']."'>text version</a>, ";
if (trim($row['htmllink']) <> "") $links=$links."<a href='".$row['htmllink']."'>html version</a>";
if ($links == "") {
$links = $links."<br>";
} else {
$links = $links."<br><br>";
} } else {
$links = "<br>";
}

$moddate = date("l, F jS, Y",$row['modifieddate']);
$projectid = $row['projectid'];

if ($type == "Gold") {
$moddate = "Uploaded: ".$moddate;
} elseif ($type == "Silver") {
$moddate = "Last Proofed: ".$moddate;
$pages = bookpages($projectid, "!=0");
$totalpages = $pages['total'];
} elseif ($type == "Bronze") {
$moddate = "Released: ".$moddate;
$pages = bookpages($projectid, "!=0");
$totalpages = $pages['total'];
}

echo "<font face='Verdana' size='1' color='#444444'><b>$numofetexts) \"".$row['nameofwork']."\"</b></font><font face='Verdana' size='1'>, ".$row['authorsname']."<br>";

if ($type != "Gold") echo "$totalpages pages; ";

echo "$moddate<br>$links</font>";

$numofetexts++;
}
?>

</body>
</html>
