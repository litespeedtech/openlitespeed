<?
if (!defined('LEGAL')) return;
echo '<h2>' . TITLE . '</h2>';

$manual_script = $check->pass_val['manual_script'];
if ($manual_script == NULL) {// illegal entry
	return; 
}
$binname = 'lsphp-' . $check->pass_val['php_version'];	

$notes = '<ul><li>If the build process is successful, the PHP binary will be created under '
	. $_SERVER['LS_SERVER_ROOT'] . '/fcgi-bin/ with name ' . $binname
	. ', and a symbolic link of lsphp' . $check->pass_val['php_version'][0] 
	. ' will be created or updated to the newly built binary. If '
	. $binname . ' already exists, it will be renamed to ' . $binname . '.bak.</li>';
	
if ( $check->pass_val['extentions'] != '') {
	$notes1 = BuildTool::getExtensionNotes($check->pass_val['extentions']);
	$notes .= "\n" . $notes1 . '</ul>';	
}

$echo_cmd = 'echo "For security reason, please log in and manually run the pre-generated script to continue."';
exec($echo_cmd . ' > ' . $check->pass_val['log_file']); 
exec($echo_cmd . ' > ' . $check->pass_val['progress_file']); 

?>

<div class="panel_error" align=left>
<p class="gui_error" id="errorzone">For security reason, please log in to your server and run the pre-generated script from shell. You can monitor the progress from this screen.</p>
<p>If you log in as root, you can directly run the command:</p> 
<p class="panel_shell"># <?=$manual_script ?></p>
<p>If you log in as a user who has sudo permission, you can run the command with sudo and input root password after prompt. </p>
<p class="panel_shell">$ sudo <?=$manual_script ?></p>
</div>

<div class="xtbl_header"><p class="xtbl_title">
	Step 4 : Monitoring build process  
</p>
</div>

<script>
function getUpdate() {
	try { xhr = new XMLHttpRequest(); }
	catch(e)
	{
	    xhr = new ActiveXObject(Microsoft.XMLHTTP);
	}
	
	xhr.abort();
	xhr.open("GET", "buildProgress.php", true);
	xhr.send(null);

	xhr.onreadystatechange = function() {

	    if(xhr.readyState != 4)
	    	return;
    
		if(xhr.status == 200)
		{
			var ta = document.getElementById("statuszone");
			var pos = xhr.responseText.indexOf("\n**LOG_DETAIL**");
        	ta.innerHTML = xhr.responseText.substring(0,pos);
        	
        	var loga = document.getElementById("logzone");
        	loga.innerHTML = xhr.responseText.substring(pos);
        	if (xhr.responseText.indexOf("\n**COMPLETE**") >= 0) {
	        	document.getElementById("statusgraphzone").innerHTML = '<b>Build PHP Completed!</b> To apply changes, please visit Control Panel and execute a Graceful Restart. <a href="/service/serviceMgr.php">Apply Changes</a>';
	        	document.getElementById("errorzone").innerHTML = 'Please make sure your php.ini will work with the extension, see details in notes.';  
        	}
        	else if (xhr.responseText.indexOf("\n**ERROR**") >= 0) {
	        	document.getElementById("statusgraphzone").innerHTML = '<b>Stopped due to Error. Please manually fix it. It may due to missing packages, after you install it, rerun the same command and this page will show the updated log.</b>';
	        	setTimeout("getUpdate();", 10000);
        	}
        	else {
            	setTimeout("getUpdate();", 2000);
			}
      	} else {
        	document.getElementById("statuszone").innerHTML = xhr.responseText
        		+ "<br>Status update refresh error " + xhr.status + "... please wait";
			setTimeout("getUpdate();", 2000);
      	}
	};
}

getUpdate();

</script>

<div><span id="statusgraphzone"><img src='/static/images/working.gif'></span></div>


<p> <b>Notes</b>: <?=$notes ?></p>

<p><b>Main Status: </b></p>
<div style="margin-left:20;margin-top:0;border:1;width:900px;height:160px;overflow:auto;">
<pre id="statuszone"></pre>
</div>
<p><b>Detailed Log: </b></p>
<div style="margin-left:20;margin-top:0;border:1;width:900px;height:500px;overflow:auto;">
<pre id="logzone"></pre>
</div>
<p></p>


