var navname,navvers,newsnavc=0,sidebarcnt;

navname=navigator.appName;
navvers=parseInt(navigator.appVersion);

js_ok = ((navname == "Microsoft Internet Explorer") && (navvers >= 4 )) || ((navname == "Netscape") && (navvers >= 4 ));

if (js_ok) {
	bullet_on=new Image(102,16);
	bullet_on.src="http://img.dpreview.com/images/indicator.gif";

	bullet_off=new Image(102,16);
	bullet_off.src="http://img.dpreview.com/images/indicator_off.gif";
}

function dlimg(imgloc) {
	if(confirm("WARNING:\nYou are about to download a very large file,\nplease treat our (limited) bandwidth with respect\nand only download the original image if you\nreally need to do so. Click on cancel to stop now.")) {
		window.open(imgloc,'askeyimage');
	}
}

function dlimgt(imgloc,targ) {
	if(confirm("WARNING:\nYou are about to download a very large file,\nplease treat our (limited) bandwidth with respect\nand only download the original image if you\nreally need to do so. Click on cancel to stop now.")) {
		window.open(imgloc,targ);
	}
}


function addbookmark() {
 if (document.all) window.external.AddFavorite("http://www.dpreview.com/","dpreview.com - Digital Photography Review");
}

function menuch(sel,hilo) {
	if (js_ok) {
		toeval='';

		if (hilo==1) {
			toeval='document.'+sel+'.src=bullet_on.src;\n';
		} else {
			toeval='document.'+sel+'.src=bullet_off.src;\n';
		}
		if (toeval!='') eval(toeval);
	}
}

function help_popup(section) {
	window.location='/learn/key=' + section;
}

function fresh(time) {
	setTimeout("freshnow()",time*1000);
}

function freshnow() {
	window.location.reload();
}

function jumpto(me) {
	meval=me.options[me.selectedIndex].value;
	if(meval!="") window.location=meval;
}

function inputswap(me,def,setto) {
	if(js_ok) {
		if(me.value==def) {
			me.value=setto;
		}
	}
}

function menuitem(href,alt) {
	name='sidebar'+sidebarcnt;
	sidebarcnt++;
	document.write('<img src="/images/one.gif" width="5" height="1"><br>');
	document.write('<a href="'+href+'"');
	document.write(' onmouseout="menuch(\''+name+'\',0);" onmouseover="menuch(\''+name+'\',1);">');
	document.write('<img src="/images/one.gif" width="92" height="16" hspace="6" border="0" name="'+name+'"');
	document.write('alt="Click for: '+alt+'"></a><br>');
}

function makesidebar() {
	sidebarcnt=0
	menuitem('/','Latest News');
	menuitem('/reviews/','Reviews');
	menuitem('/reviews/specs.asp','Cameras');
	menuitem('/reviews/timeline.asp','Timeline');
	menuitem('/reviews/compare.asp','Buying Guide');
	menuitem('/reviews/sidebyside.asp','Side-by-Side');
	menuitem('/gallery/','Galleries');
	menuitem('/learn/','Learn');
	menuitem('/learn/glossary/','Glossary');
	menuitem('/forums/','Forums');
	menuitem('/misc/feedback.asp','Feedback');
	menuitem('/news/newsletter_subscribe.asp','Newsletter');
	menuitem('/misc/search.asp','Site Search');
	menuitem('/misc/links.asp','Links');
	menuitem('/misc/advertising.asp','Advertising');
	menuitem('/misc/about.asp','About Us');
}

function newsnavcell(href,title) {
	var ahref;
	
	ahref='<a href="/news/' + href +'">';
	
	document.writeln('<td class="tdnewsnav">&nbsp;' + ahref + title + '</a></td>');
}