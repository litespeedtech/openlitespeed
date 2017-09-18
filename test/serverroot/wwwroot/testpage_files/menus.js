// dpreview.com DropDown menus v1.1  -  July/2001
// Script (c)2001 Phil Askey, Digital Photography Review, http://www.dpreview.com/

menuopen=-1;
menuurlprefix='';

function findObj(n) {
	var x;
	
	d=document;
	if(!(x=d[n])&&d.all) x=d.all[n];
	if(!x && d.getElementById) x=d.getElementById(n);
	return x;
}

function hidemenu(menuid) {
	menuobj=findObj("menu"+menuid)
	menuheadobj=findObj("menuhead"+menuid)

	if (menuobj) {
		menuobj.style.visibility='hidden';
	}
	menuheadobj.className='menuhead';

	menuopen=-1;
}

function showmenu(menuid) {
	menuobj=findObj("menu"+menuid);
	menuheadobj=findObj("menuhead"+menuid)

	if (menuobj) {
		dh=document.body.clientHeight;
		menupos=menuheadobj.parentElement.offsetTop-document.body.scrollTop;
		menuheight=menuobj.clientHeight;
		if((menupos+menuheight>dh)&&(menupos>0)) {
			menutop=dh-(menuheight+menupos)-2;
			if (menutop<-menupos) menutop=-menupos;
			menuobj.style.top=menutop;
		} else {
			menuobj.style.top=0;
		}
		
		menuobj.style.visibility='visible';
		menuheadobj.className='menuhead_opensub';
	} else {
		menuheadobj.className='menuhead_open';
	}

	menuopen=menuid;
}

function menuhead_rollon() {
	o=window.event.srcElement;
	openmenuid=o.id.substr(8);

	if(menuopen>=0) hidemenu(menuopen);

	showmenu(openmenuid);
}

function menu_rollon() {
	o=window.event.srcElement;
	if (o.className == "menuopt") o.className = "menuopt_over";
}

function menu_rolloff() {
	o=window.event.srcElement;
  	if (o.className.substring(0,7) == "menuopt") o.className = "menuopt";
}

function menu_mousedown() {
	o=window.event.srcElement;
	if (o.className == "menuopt_over") o.className = "menuopt_click";
 	if (o.className == "menuhead_open") o.className = "menuhead_click";
 	if (o.className == "menuhead_opensub") o.className = "menuhead_clicksub";
}

function menu_mouseup() {
	o=window.event.srcElement;
 	if (o.className == "menuopt_click") o.className = "menuopt_over";
 	if (o.className == "menuhead_click") o.className = "menuhead_open";
 	if (o.className == "menuhead_clicksub") o.className = "menuhead_opensub";
}

function menu_mouseclick() {
	o=window.event.srcElement;
	if (o.className == "menuopt_over") {
		menusubid=o.id.split(".");
		menuid=menusubid[0];
		menuoptid=menusubid[1];
		t=menuoptions[menuid][menuoptid].split('|');
		newurl=t[1];
		if (newurl.substr(0,7)!='http://') newurl=menuurlprefix + newurl;
		window.location=newurl;
	} else if ((o.className == "menuhead_open")||(o.className == "menuhead_opensub")) {
		menuid=o.id.substr(8);
		t=menuoptions[menuid][0].split('|');
		newurl=t[1];
		if (newurl.substr(0,7)!='http://') newurl=menuurlprefix + newurl;
		window.location=newurl;
	}
}

function document_mouseover() {
	if(menuopen>=0) {
		o=window.event.srcElement;
		menuobj=findObj("menu"+menuopen);
		if (menuobj) {
			if ((menuobj.style.visibility == "visible")&&(o.id.length==0)) hidemenu(menuopen);
		} else {
			menuobj=findObj("menuhead"+menuopen);
			if ((menuobj.className == "menuhead_open")&&(o.id.length==0)) menuobj.className="menuhead";
		}
	}
}

function menu_build() {
	for (x in menuoptions) {
		subopts=menuoptions[x].length;
		vpos=72+(x*17);
		for (y in menuoptions[x]) {
			t=menuoptions[x][y].split('|');
			if(y==0) {
				document.write ('<span class="menuspan" style="position: absolute; top: ' + vpos + ';">&nbsp;&nbsp;');
				document.writeln ('<span class="menuhead" id="menuhead'+x+'" unselectable="on">'+t[0]+'</span>');
				if(subopts>1) document.writeln ('<span id="menu'+x+'" class="menu"><table width="'+t[2]+'" border="0" cellspacing="0" cellpadding="0">');
			} else {
				document.writeln ('<tr><td class="menuopt" id="'+x+'.'+y+'" unselectable="on">'+t[0]+'</td></tr>');
			}
		}
		if(subopts>1) document.writeln('</table></span>');
		document.write('</span>');
	}
}

function menu_addevents() {
	for (x in menuoptions) {
		eval('menuhead'+x+'.onmouseover = menuhead_rollon');
		eval('menuhead'+x+'.onmousedown = menu_mousedown');
		eval('menuhead'+x+'.onmouseup = menu_mouseup');
		eval('menuhead'+x+'.onclick = menu_mouseclick');
		if (menuoptions[x].length>1) {
			eval('menu'+x+'.onmouseout = menu_rolloff');
			eval('menu'+x+'.onmouseover = menu_rollon');
			eval('menu'+x+'.onmousedown = menu_mousedown');
			eval('menu'+x+'.onmouseup = menu_mouseup');
			eval('menu'+x+'.onclick = menu_mouseclick');
		}
	}
	document.onmouseover=document_mouseover;
}

// Script (c)2001 Phil Askey, Digital Photography Review, http://www.dpreview.com/