// New version
// start of code by Carel

docRef=null;
markRef=null;
cRef=null;
// true=fancy : false=plain
cnSel=null;

//store fancy data
curSel='';
curCaret='';

// extended charset
//extC=' ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ$';
extCA=new Array(' ','¡','¢','£','¤','¥','¦','§','¨','©','ª','«','¬','­','®','¯','°','±','²','³','´','µ','¶','·','¸','¹','º','»','¼','½','¾','¿','À','Á','Â','Ã','Ä','Å','Æ','Ç','È','É','Ê','Ë','Ì','Í','Î','Ï','Ð','Ñ','Ò','Ó','Ô','Õ','Ö','×','Ø','Ù','Ú','Û','Ü','Ý','Þ','ß','à','á','â','ã','ä','å','æ','ç','è','é','ê','ë','ì','í','î','ï','ð','ñ','ò','ó','ô','õ','ö','÷','ø','ù','ú','û','ü','ý','þ','ÿ','$');

// image width
iW='1000';

// image actual width
cW='0';
// image copy for width
var imageCopy = new Image();
imageCopy.onload = loadImageSize;

function selBox(wBox)
{
if (wBox=='char')
{cRef.markBoxChar.focus();
cRef.markBoxChar.select();
cRef.tCharsA.selectedIndex=0;
cRef.tCharsE.selectedIndex=0;
cRef.tCharsI.selectedIndex=0;
cRef.tCharsO.selectedIndex=0;
cRef.tCharsU.selectedIndex=0;
cRef.tCharsM.selectedIndex=0;
cRef.tCharsC.selectedIndex=0;
cRef.tCharsD.selectedIndex=0;
cRef.tCharsS.selectedIndex=0;
cRef.tCharsZ.selectedIndex=0;
cRef.tCharsCyr.selectedIndex=0;
cRef.tCharsOCyr.selectedIndex=0;}
else if (wBox=='start')
{markRef.markBox.focus();
markRef.markBox.select();
markRef.ttagsMore.selectedIndex=0;}
else if (wBox=='oldS')
{markRef.markBox.focus();
markRef.markBox.select();}
else if (wBox=='oldE')
{markRef.markBoxEnd.focus();
markRef.markBoxEnd.select();}
}

function getCurSel()
{if (cnSel){curSel=docRef.selection.createRange().text;}}

function getCurCaret()
{if (cnSel){docRef.editform.text_data.caretPos=docRef.selection.createRange().duplicate();}}

// gets character code from numeric value cC
function gCC(cC)
{thisC=String.fromCharCode(cC);
if (thisC.length != 1)
{if (cC !=36)
{thisC=extCA[cC-160];}
else {thisC='$';}
}
return thisC;}

// fancy check for selection
function chkRange()
{if (cnSel){return (!docRef.editform.text_data.createTextRange + !docRef.editform.text_data.caretPos)? false:true;}else {return false;}}

//fancy places text cT at caret position
function putCT(cT)
{
curCaret=docRef.editform.text_data.caretPos;
curCaret.text=cT;
curSel='';
curCaret='';
docRef.editform.text_data.focus();
}

// 2 - 97
mUC=new Array(0,0,gCC(161),gCC(162),gCC(163),gCC(164),
gCC(165),gCC(166),gCC(167),gCC(168),gCC(169),
gCC(170),gCC(171),gCC(172),gCC(173),gCC(174),
gCC(175),gCC(176),gCC(177),gCC(178),gCC(179),
gCC(180),gCC(181),gCC(182),gCC(183),gCC(184),
gCC(185),gCC(186),gCC(187),gCC(188),gCC(189),
gCC(190),gCC(191),gCC(192),gCC(193),gCC(194),
gCC(195),gCC(196),gCC(197),gCC(198),gCC(199),
gCC(200),gCC(201),gCC(202),gCC(203),gCC(204),
gCC(205),gCC(206),gCC(207),gCC(208),gCC(209),
gCC(210),gCC(211),gCC(212),gCC(213),gCC(214),
gCC(215),gCC(216),gCC(217),gCC(218),gCC(219),
gCC(220),gCC(221),gCC(222),gCC(223),gCC(224),
gCC(225),gCC(226),gCC(227),gCC(228),gCC(229),
gCC(230),gCC(231),gCC(232),gCC(233),gCC(234),
gCC(235),gCC(236),gCC(237),gCC(238),gCC(239),
gCC(240),gCC(241),gCC(242),gCC(243),gCC(244),
gCC(245),gCC(246),gCC(247),gCC(248),gCC(249),
gCC(250),gCC(251),gCC(252),gCC(253),gCC(254),
gCC(255),gCC(036));

mUO=new Array();
mUO[1]='blank page';
mUO[20]='p';
mUO[21]='i';
mUO[22]='b';
mUO[23]='u';
mUO[24]='caps';
mUO[25]='sup';
mUO[26]='sub';
mUO[27]='footnote';
mUO[28]='endnote';
mUO[29]='sidenote';
mUO[30]='illustration';
mUO[31]='poetry';
mUO[32]='drama';
mUO[33]='lyrics';
mUO[34]='letter';
mUO[35]='blockquote';
mUO[36]='table';
mUO[37]='formatted';
mUO[38]='formula';
mUO[39]='math';
mUO[40]='glossary';
mUO[41]='term';
mUO[42]='definition';
mUO[43]='bibliography';
mUO[44]='header';

// character selection
function iMUc(wM)
{
 if (inProof==1)
 {
cRef.markBoxChar.value=String.fromCharCode(wM);//mUC[wM];
cR=chkRange();

//plain
if (!cnSel || !cR)
{selBox('char');}

//fancy
if (cR)
{cT=String.fromCharCode(wM);//mUC[wM];
putCT(cT);}
 }
}

function new_iMUc(wM)
{
cRef.tCharsA.selectedIndex=0;
cRef.tCharsE.selectedIndex=0;
cRef.tCharsI.selectedIndex=0;
cRef.tCharsO.selectedIndex=0;
cRef.tCharsU.selectedIndex=0;
cRef.tCharsM.selectedIndex=0;
cRef.tCharsC.selectedIndex=0;
cRef.tCharsD.selectedIndex=0;
cRef.tCharsS.selectedIndex=0;
cRef.tCharsZ.selectedIndex=0;
cRef.tCharsCyr.selectedIndex=0;
cRef.tCharsOCyr.selectedIndex=0;

cRef.markBoxChar.value=String.fromCharCode(wM);
insertTags(String.fromCharCode(wM),'','');
}

// standard tag selection
function iMU(wM)
{
if (inProof==1) {
	wTag=mUO[wM];
	wOT='<'+wTag+'>';
	wCT='';

	if (wM > 19) {
		wCT='</'+wTag+'>';
	}

	markRef.markBox.value=wOT;
	markRef.markBoxEnd.value=wCT;
	//markRef.ttagsMore.selectedIndex=0;
	cR=chkRange();

	//plain
	if (!cnSel || !cR) {
		selBox('old');
	}

	getCurSel();
	//fancy
	if (docRef.editform.text_data.selectionEnd && (docRef.editform.text_data.selectionEnd - docRef.editform.text_data.selectionStart > 0)) {
		mozWrap(docRef.editform.text_data, wOT, wCT);
		//this block based on phpBB2
	} else if (curSel != '' && docRef.selection.createRange().text == curSel) {
		docRef.editform.text_data.focus();
		docRef.selection.createRange().text=wOT + curSel + wCT;
		curCaret='';
		curSel='';
		docRef.editform.text_data.focus();
	} else { 
		if (cR && curSel=='') {
			cT=wOT;
			putCT(cT);
		}
	}

	if(wM==1) {
		docRef.editform.text_data.value=wOT;
	}
}

}

function new_iMU(wOT,wCT)
{
markRef.markBox.value=wOT;
markRef.markBoxEnd.value=wCT;

insertTags(wOT,wCT,'');
}

// start of general interface functions

function mGR()
{
	// greek character window
	winURL='greek2ascii.php';
	newFeatures="'toolbars=0,location=0,directories=0;status=0;menubar=0,scrollbars=1,resizable=1,width=640,height=210,top=180,left=180'";
	greekWin=window.open(winURL,"gkasciiWin",newFeatures);
	greekWin.focus();
}

// Font Face Selection values
aFnt=new Array();
aFnt[1]='Courier New';
aFnt[2]='Times New Roman';
aFnt[3]='Arial';
aFnt[4]='Lucida Sans Typewriter';
aFnt[5]='monospace';
aFnt[6]='DPCustomMono2';
aFnt[7]='Courier';	// re-added per Task 400

// Font Size Selection values
bFnt=new Array();
bFnt[1]='8';
bFnt[2]='9';
bFnt[3]='10';
bFnt[4]='11';
bFnt[5]='12';
bFnt[6]='13';
bFnt[7]='14';
bFnt[8]='15';
bFnt[9]='16';
bFnt[10]='18';
bFnt[11]='20';

ieW=0;
ieH=0;
ieL=0;
ieT=0;
ieSt=0;

function setText()
{
//if (document.all && !ieSt)
if (!ieSt)
	{
	ieW=docRef.editform.text_data.style.width;
	ieH=docRef.editform.text_data.style.height;
	ieSt=1;
	}
}

function fixText()
{
//if (document.all)
//{
docRef.editform.text_data.style.width=ieW;
docRef.editform.text_data.style.height=ieH;
//}
}

function chFFace(fF)
{if(parseInt(fF)){setText();docRef.editform.text_data.style.fontFamily=aFnt[fF];
fixText();}}

function chFSize(fS)
{if(parseInt(fS)){setText();docRef.editform.text_data.style.fontSize=bFnt[fS]+'pt';
fixText();}}

function showAllText()
{alert(docRef.editform.text_data.value);}

function showIZ()
{
nP=docRef.editform.zmSize.value;
zP=Math.round(iW*(nP/100));
reSize(zP)
docRef.editform.zmSize.value=nP;
return false;
}

function showActual()
{
  docRef.editform.zmSize.value = cW/10;
  return showIZ();
}

function loadImageSize()
{
  if (imageCopy.complete) {
    // This needs to be fixed properly.
    // There is a varying maximum limit to image size, above which the
    // image vanishes from the proofing interface.  Don't know why, yet.
    if (imageCopy.width > 2000) {
      cW = 2000;
    } else {
      cW = imageCopy.width;
    }
  }
}

function makeImageCopy()
{
  imageCopy.src = frameRef.scanimage.src;
}

function showNW()
{
nW=window.open();
nW.document.open();
// SENDING PAGE-TEXT TO USER
// We're sending it in a HTML document,
// so we entity-encode its HTML-special characters.
nW.document.write('<PRE>'+showNW_safe(docRef.editform.text_data.value)+'</PRE>');
nW.document.close()
}

function showNW_safe(str)
// Entity-encode str's HTML-special characters,
// but reinstate <i> and <b> and <hr> tags,
// because we want the browser to interpret them (but nothing else) as markup.
{
    return html_safe(str)
	.replace(/&lt;(\/?)(i|b|hr)&gt;/ig, '<$1$2>')
}

function html_safe(str)
// Return a version of str that is safe to send as element-content
// in an HTML document.
// That is, make the following replacements:
//    &  ->  &amp;
//    <  ->  &lt;
//    >  ->  &gt;
// This should be equivalent to PHP's
//     htmlspecialchars($str,ENT_NOQUOTES)
{
    return str
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
}

function dSI(sdir)
{
// Modified from the script by Philip Serracino Inglott

targ=top.imageframe.window;
ammt =20; 		// this is the amount to scroll in pixels
			// should be user configurable
switch (sdir) {
  case "up" :
   targ.scrollBy(0,-ammt);
   break;
  case "down" :
   targ.scrollBy(0,ammt);
   break;
  case "left" :
   targ.scrollBy(-ammt,0);
   break;
  case "right" :
   targ.scrollBy(ammt,0);
   break;
  } 
docRef.editform.text_data.focus();
return true;}


// just for the old ptags list
otO=new Array(); // Opening tags
otC=new Array(); // Closing tags
otO[0]='*';
otC[0]='';
otO[1]='[Footnote #: ';
otC[1]=']';
otO[2]='[Sidenote: ';
otC[2]=']';
otO[3]='[Illustration: ';
otC[3]=']';
otO[4]='/*'; // This index number is relied upon below
otC[4]='*/';
otO[5]='       *       *       *       *       *';
otC[5]='';
otO[6]='[Blank Page]';
otC[6]='';
otO[7]='/#'; // This index number is relied upon below
otC[7]='#/';
otO[8]='[Greek: ';
otC[8]=']';

// standard tag selection
function iMUO(wM)
{
if (inProof==1) {
	wOT=otO[wM];
	wCT=otC[wM];
	wWT=wOT;

	if (wM > 19) {
		wCT=otC[wM-20];wOT='';wWT=wCT;
	}


	markRef.markBox.value=wOT;
	markRef.markBoxEnd.value=wCT;
	cR=chkRange();

	//plain
	if (!cnSel || !cR) {
		if (wM > 19) {
			selBox('oldE');
		} else {
			selBox('oldS');
		}
	}

	getCurSel();
	//fancy
	if (docRef.editform.text_data.selectionEnd && (docRef.editform.text_data.selectionEnd - docRef.editform.text_data.selectionStart > 0)) {
		mozWrap(docRef.editform.text_data, wOT, wCT);
		//this block based on phpBB2
	} else if (curSel != '' && docRef.selection.createRange().text == curSel) {
		docRef.editform.text_data.focus();

		if ((wM==4) || (wM==7))
		{
			docRef.selection.createRange().text=wOT + '\n' + curSel + '\n' + wCT;
		} else {
			docRef.selection.createRange().text=wOT + curSel + wCT;
		}

		curCaret='';
		curSel='';
		docRef.editform.text_data.focus();
	} else { 
		if (cR && curSel=='') {
			cT=wWT;
			putCT(cT);
		}
	}

	if(wM==6) {
		docRef.editform.text_data.value=wOT;
	}
}

}


function doBU()
{
if (frameRef.scanimage) {
    makeImageCopy();
    showIZ();
  }
}

// a required var
isLded2=0;

// From http://www.massless.org/mozedit/
function mozWrap(txtarea, open, close)
{
	var selLength = txtarea.textLength;
	var selStart = txtarea.selectionStart;
	var selEnd = txtarea.selectionEnd;
	if (selEnd == 1 || selEnd == 2)
		selEnd = selLength;

	var s1 = (txtarea.value).substring(0,selStart);
	var s2 = (txtarea.value).substring(selStart, selEnd)
	var s3 = (txtarea.value).substring(selEnd, selLength);
	txtarea.value = s1 + open + s2 + close + s3;
	return;
}

// Following is taken from Wikipedia's wikibits.js:

var clientPC = navigator.userAgent.toLowerCase(); // Get client info
var is_gecko = ((clientPC.indexOf('gecko')!=-1) && (clientPC.indexOf('spoofer')==-1)
                && (clientPC.indexOf('khtml') == -1) && (clientPC.indexOf('netscape/7.0')==-1));
var is_safari = ((clientPC.indexOf('AppleWebKit')!=-1) && (clientPC.indexOf('spoofer')==-1));

// apply tagOpen/tagClose to selection in textarea,
// use sampleText instead of selection if there is none
// copied and adapted from phpBB
function insertTags(tagOpen, tagClose, sampleText) {
	var txtarea = docRef.editform.text_data;
	// IE
	if(docRef.selection  && !is_gecko) {
		var theSelection = docRef.selection.createRange().text;
		if(!theSelection) { theSelection=sampleText;}
		txtarea.focus();
		if(theSelection.charAt(theSelection.length - 1) == " "){// exclude ending space char, if any
			theSelection = theSelection.substring(0, theSelection.length - 1);
			docRef.selection.createRange().text = tagOpen + theSelection + tagClose + " ";
		} else {
			docRef.selection.createRange().text = tagOpen + theSelection + tagClose;
		}

	// Mozilla
	} else if(txtarea.selectionStart || txtarea.selectionStart == '0') {
 		var startPos = txtarea.selectionStart;
		var endPos = txtarea.selectionEnd;
		var scrollTop=txtarea.scrollTop;
		var myText = (txtarea.value).substring(startPos, endPos);
		if(!myText) { myText=sampleText;}
		if(myText.charAt(myText.length - 1) == " "){ // exclude ending space char, if any
			subst = tagOpen + myText.substring(0, (myText.length - 1)) + tagClose + " ";
		} else {
			subst = tagOpen + myText + tagClose;
		}
		txtarea.value = txtarea.value.substring(0, startPos) + subst +
		  txtarea.value.substring(endPos, txtarea.value.length);
		txtarea.focus();

		var cPos=startPos+(tagOpen.length+myText.length+tagClose.length);
		txtarea.selectionStart=cPos;
		txtarea.selectionEnd=cPos;
		txtarea.scrollTop=scrollTop;

	// All others
	} else {
		var copy_alertText=alertText;
		var re1=new RegExp("\\$1","g");
		var re2=new RegExp("\\$2","g");
		copy_alertText=copy_alertText.replace(re1,sampleText);
		copy_alertText=copy_alertText.replace(re2,tagOpen+sampleText+tagClose);
		var text;
		if (sampleText) {
			text=prompt(copy_alertText);
		} else {
			text="";
		}
		if(!text) { text=sampleText;}
		text=tagOpen+text+tagClose;
		docRef.infoform.infobox.value=text;
		// in Safari this causes scrolling
		if(!is_safari) {
			txtarea.focus();
		}
		noOverwrite=true;
	}
	// reposition cursor if possible
	if (txtarea.createTextRange) txtarea.caretPos = docRef.selection.createRange().duplicate();
}
