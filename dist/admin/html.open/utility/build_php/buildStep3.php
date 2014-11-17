<?
if (!defined('LEGAL')) return;
echo '<h2 class="bottom_bar">' . TITLE . '</h2>';

$options = $check->pass_val['build_options'];
if ($options == NULL) // illegal entry
	return;

$err = '';
$tool = new BuildTool($options);
if (!$tool || !$tool->generate_script($err))
{
	echo '<div class="panel_error" align=left><span class="gui_error">Fail to generate build script, please try to manually fix the error first.<br>'.
		$err . '</span></div>';
	return;
}


$_SESSION['progress_file'] = $tool->progress_file;
$_SESSION['log_file'] = $tool->log_file;

$cmd = 'bash -c "exec ' . $tool->build_prepare_script . ' 1> ' . $tool->log_file . ' 2>&1" &';
exec($cmd);

?>

<div class="panel_error" align=left><span class="gui_error" id="errorzone">Please do not use the browser refresh, back and forward buttons while building PHP.</span></div>

<div class="xtbl_header"><p class="xtbl_title">
	Step 3 : Preparing for building PHP <?php echo $options->GetValue('PHPVersion');?> Binary
</p>
</div>
<form name="buildphp" method="post">
<input type="hidden" name="step" value="3">
<input type="hidden" name="manual_script", value="<?=$tool->build_manual_run_script?>">
<input type="hidden" name="extentions" value="<?=$tool->extension_used?>">
<input type="hidden" name="php_version" value="<?php echo $options->GetValue('PHPVersion');?>">

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
        	if (xhr.responseText.indexOf("\n**PREPARE_DONE**") >= 0) {
	        	document.getElementById("statusgraphzone").innerHTML = 'Preparation finished successfully.&nbsp;&nbsp;&nbsp;\
	        		<input type="submit" name="back" value="Back to Step 2 to change build options">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\
	        		<input type="submit" name="buildsubmit" value="Next">';
        	}
        	else if (xhr.responseText.indexOf("\n**ERROR**") >= 0) {
	        	document.getElementById("statusgraphzone").innerHTML = '<b>Stopped due to Error. Please manually fix it.</b>';
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

<p><b>Main Status: </b></p>
<div style="margin:10px;border:1px solid rgb(134,142,167);outline: 3px solid #eaeaea;width:900px;height:160px;overflow:auto;font-family:Courier, monospace; font-size: 11px; background: #FFFFF7; color: #333333; padding: 5px 10px;">
<pre id="statuszone"></pre>
</div>
<p><b>Detailed Log: </b></p>
<div style="margin:10px;border:1px solid rgb(134,142,167);outline: 3px solid #eaeaea;width:900px;height:500px;overflow:auto;font-family:Courier, monospace; font-size: 11px; background: #222222; color: #ffffff; padding: 5px 10px;">
<pre id="logzone"></pre>
</div>


</form>
