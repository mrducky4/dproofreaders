<?
$relPath="./../../pinc/";
include_once($relPath.'v_site.inc');
include($relPath.'dp_main.inc');
include_once($relPath.'c_pages.inc');
include_once($relPath.'v_keepmarkup.inc');

/* $_POST $imagefile, $fileid, $proofstate, $button1, $button2, $button3, $button4,
          $projectname, $text_data, $orient, $lang, $js, $button1_x, $button2_x,
          $button3_x, $button4_x, $editone, $savedm $pagestate */

if (!isset($projectname))
{
    echo "Error: processtext.php: \$projectname is not set.";
    exit;
}
if ($projectname == '')
{
    echo "Error: processtext.php: \$projectname is empty.";
    exit;
}
$project = $projectname;

$text_data = isset($text_data) ? $text_data : '';

$tpage=new processpage();
$tpage->setPageState($pagestate,$project,$fileid,$imagefile,$proofstate);

define('B_TEMPSAVE',                1);
define('B_SAVE_AND_DO_ANOTHER',     2);
define('B_QUIT',                    3);
define('B_SWITCH_LAYOUT',           4);
define('B_SAVE_AND_QUIT',           5);
define('B_REPORT_BAD_PAGE',         6);
define('B_RETURN_PAGE_TO_ROUND',    7);
define('B_REVERT_TO_ORIGINAL',      8);
define('B_REVERT_TO_LAST_TEMPSAVE', 9);
define('B_RUN_SPELL_CHECK',         10);
define('B_RUN_COMMON_ERRORS_CHECK', 11);


// set tbutton
  if (isset($button1) || isset($button1_x)) {$tbutton=B_TEMPSAVE;}
  if (isset($button2) || isset($button2_x)) {$tbutton=B_SAVE_AND_DO_ANOTHER;}
  if (isset($button3) || isset($button3_x)) {$tbutton=B_QUIT;}
  if (isset($button4) || isset($button4_x)) {$tbutton=B_SWITCH_LAYOUT;}
  if (isset($button5) || isset($button5_x)) {$tbutton=B_SAVE_AND_QUIT;}
  if (isset($button6) || isset($button6_x)) {$tbutton=B_REPORT_BAD_PAGE;}
  if (isset($button7) || isset($button7_x)) {$tbutton=B_RETURN_PAGE_TO_ROUND;}
  if (isset($button8) || isset($button8_x)) {$tbutton=B_REVERT_TO_ORIGINAL;}
  if (isset($button9) || isset($button9_x)) {$tbutton=B_REVERT_TO_LAST_TEMPSAVE;}
  if (isset($button10) || isset($button10_x)) {$tbutton=B_RUN_SPELL_CHECK;}
  if (isset($button11) || isset($button11_x)) {$tbutton=B_RUN_COMMON_ERRORS_CHECK;}

  if (isset($spcorrect)) {$tbutton=101;} // Make Spelling Corrections
  if (isset($spexit)) {$tbutton=102;} // Exit Spelling Corrections
  if (isset($errcorrect)) {$tbutton=111;} // Make Spelling Corrections
  if (isset($errexit)) {$tbutton=112;} // Exit Spelling Corrections

// set prefs
  if ($userP['i_type']==1)
  {
    $isChg=0;
      if ($userP['i_layout']==1)
      {
        if (isset($fntFace) && $userP['v_fntf']!=$fntFace) {$userP['v_fntf']=$fntFace;$isChg=1;}
        if (isset($fntSize) && $userP['v_fnts']!=$fntSize) {$userP['v_fnts']=$fntSize;$isChg=1;}
        if (isset($zmSize) && $userP['v_zoom']!=$zmSize) {$userP['v_zoom']=$zmSize;$isChg=1;}
      }
      else
      {
        if (isset($fntFace) && $userP['h_fntf']!=$fntFace) {$userP['h_fntf']=$fntFace;$isChg=1;}
        if (isset($fntSize) && $userP['h_fnts']!=$fntSize) {$userP['h_fnts']=$fntSize;$isChg=1;}
        if (isset($zmSize) && $userP['h_zoom']!=$zmSize) {$userP['h_zoom']=$zmSize;$isChg=1;}
      }
    $userP['prefschanged']=$isChg;
    $cookieC->setTempPrefs($userP,$pguser);
  }

//Make sure project is still available
  // only if not in a check
  if ($tbutton <100 && $project !=0)
  {
    $sql = "SELECT state FROM projects WHERE projectid = '$project' LIMIT 1";
    $result = mysql_query($sql);
    $state = mysql_result($result, 0, "state");
      if ((($proofstate == PROJ_PROOF_FIRST_AVAILABLE) && ($state != AVAIL_FIRST)) ||
          (($proofstate == PROJ_PROOF_SECOND_AVAILABLE) && ($state != AVAIL_SECOND)))
      {
        $tpage->noPages($userP['i_newwin']);
        exit;
      } // end project open check
  }

// BUTTON CODE

// temp saves and revert
if ($tbutton==B_TEMPSAVE || $tbutton==B_SWITCH_LAYOUT || $tbutton==B_REVERT_TO_ORIGINAL || $tbutton==B_REVERT_TO_LAST_TEMPSAVE)
{
  $npage=$tpage->getPageCookie();
  if ($tbutton!=B_REVERT_TO_LAST_TEMPSAVE) {$npage['pagestate']=$tpage->saveTemp($proofstate,$text_data,$pguser);}
  else {$npage['pagestate']=$tpage->getRevertState();}
    if ($tbutton==B_SWITCH_LAYOUT)
    {
      $userP['i_layout']=$userP['i_layout']==1? 0:1;
      $userP['prefschanged']=1;
      $cookieC->setTempPrefs($userP, $pguser);
    } // end change layout prefs
    if ($tbutton==B_REVERT_TO_ORIGINAL) {$npage['revert']=1;}
    else {$npage['revert']=0;}
  $npage['saved']=1;
  $npage['spcheck']=0;
  $npage['errcheck']=0;
  $tpage->setTempPageCookie($npage);
  if ($userP['i_type'] != 1)
    {include('proof_frame_nj.inc');}
  else
    {metarefresh(0,"text_frame.php","Proofing Text Frame","Loading next available page....");}
  exit;
} // end B_TEMPSAVE B_SWITCH_LAYOUT B_REVERT_TO_ORIGINAL B_REVERT_TO_LAST_TEMPSAVE

// =============================================================================

if ($tbutton==B_SAVE_AND_DO_ANOTHER || $tbutton==B_SAVE_AND_QUIT)
{
	$tpage->saveComplete($proofstate,$text_data,$pguser,$userP);
}
else if ($tbutton==B_RETURN_PAGE_TO_ROUND)
{
	$tpage->returnPage($proofstate,$pguser,$userP);
}

if ($tbutton==B_SAVE_AND_DO_ANOTHER)
{
	$url = "proof_frame.php?project=$project&amp;proofstate=$proofstate";
	metarefresh(1,$url,'Save and Do Next Page','Page saved.');
}
else if ($tbutton==B_QUIT || $tbutton==B_SAVE_AND_QUIT || $tbutton==B_RETURN_PAGE_TO_ROUND)
{
	if ($tbutton==B_QUIT)
	{
		$title='Quit Proofing';
		$body='';
	}
	else if ($tbutton==B_SAVE_AND_QUIT)
	{
		$title='Save and Quit Proofing';
		$body='Page Saved. ';
	}
	else if ($tbutton==B_RETURN_PAGE_TO_ROUND)
	{
		$title='Return to Round';
		$body='Page Returned to Round. ';
	}
	$body .= 'Exiting proofing interface....';
	$url = "projects.php?project=$project&amp;proofstate=$proofstate";
	metarefresh(1,$url,$title,$body);

	// $editone=isset($editone)?$editone:0;
	// $tpage->exitInterface($userP['i_newwin'],$editone);
}

// =============================================================================

if ($tbutton==B_REPORT_BAD_PAGE)
{
$badState=$tpage->bad_page;
include('badpage.php');
} // end B_REPORT_BAD_PAGE

if ($tbutton==B_RUN_SPELL_CHECK)
{
  $npage=$tpage->getPageCookie();
  $npage['spcheck']=1;
  $tpage->setTempPageCookie($npage);
  include('spellcheck.inc');
} // end B_RUN_SPELL_CHECK

// run common errors check
if ($tbutton==B_RUN_COMMON_ERRORS_CHECK)
{
  $npage=$tpage->getPageCookie();
  $npage['errcheck']=1;
  $tpage->setTempPageCookie($npage);
//  include('errcheck.inc');
} // end B_RUN_COMMON_ERRORS_CHECK

// Return from spellcheck page...
if ($tbutton==101 || $tbutton==102)
{
    include_once('spellcheck_text.inc');

    if ( $tbutton == 101 )
    {
	// User hit "Submit Corrections" button.
	$correct_text = spellcheck_apply_corrections();

	if ($userP['i_type']==0)
	{
	  $tpage=new processpage();
	  $npage=$tpage->getPageCookie();
	  $npage['spcheck']=2;
	  $tpage->setTempPageCookie($npage);
	}
    }
    else if ( $tbutton == 102 )
    {
	// User hit "Quit" button.
	$correct_text = spellcheck_quit();

	$npage=$tpage->getPageCookie();
	if ($userP['i_type']==1)
	  {$npage['spcheck']=0;}
	else
	  {$npage['spcheck']=2;}
	$tpage->setTempPageCookie($npage);
    }

    $inCheck=1;
    if ($userP['i_type']==1)
      {include('text_frame.php');}
    else
      {
        // write file
          $text_file= $project.substr($imagefile,0,-4).".txt";
          if ($fd=fopen("$aspell_temp_dir/$text_file","w"))
            {fwrite($fd,$correct_text);}
        include('proof_frame_nj.inc');
      }
}

?>
