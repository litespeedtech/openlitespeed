<?php
	if (!defined('LEGAL')) return;
	echo '<h2 class="bottom_bar">' . TITLE . '</h2>';

	$options = NULL;
	$saved_options = NULL;
	$default_options = NULL;
	$cur_step = $check->GetCurrentStep();

	if ($cur_step == 1) {
		$php_version = $check->pass_val['php_version'];
		$options = new BuildOptions($php_version);
		$options->setDefaultOptions();
		$default_options = $options;
        $supported = $check->GetModuleSupport($php_version);
	}
	elseif ($cur_step == 2) {
		$options = $check->pass_val['input_options'];
		$php_version = $options->GetValue('PHPVersion');
		$default_options = new BuildOptions($php_version);
		$default_options->setDefaultOptions();
	}
	elseif ($cur_step == 3) {
		$php_version = $check->pass_val['php_version'];
		$options = new BuildOptions($php_version);
		$default_options = new BuildOptions($php_version);
		$default_options->setDefaultOptions();
	}
	if ($options == NULL) return "NULL options\n";

	$saved_options = $options->getSavedOptions();
	if ($saved_options != NULL && $cur_step == 3) {
		$options = $saved_options;
	}

	if ( isset($check->pass_val['err'])) {
		echo '<div class="panel_error" align=left><span class="gui_error">Input error detected. Please resolve the error(s). </span></div>';
	}

?>

<form name="buildphp" method="post">
<input type="hidden" name="step" value="2">
<input type="hidden" name="version" value="<?=$php_version?>">

<table width="100%" class="xtbl" border="0" cellpadding="5" cellspacing="1">
	<tr class="xtbl_header">
		<td colspan="3" class="xtbl_title">
		Step 2 : Choose PHP <?=$php_version?> Build Options
		</td>
	</tr>
	<tr class="xtbl_value">
            <td class="xtbl_label">Load Configuration</td>
            <td class="icon"></td>
            <td><input type="button" value="Use Configuration from Previous Build"
            <?
            if ($saved_options == NULL) {
            	echo "disabled";
            }
            else {
            	echo $saved_options->gen_loadconf_onclick('IMPORT');
            }
            ?>
            >
            <input type="button" value="Restore Defaults"
            <? echo $default_options->gen_loadconf_onclick('DEFAULT'); ?>
            ></td>
    </tr>
    <tr class="xtbl_value">
            <td class="xtbl_label">Extra PATH environment</td>
            <td class="icon"></td>
            <td>
            <?
            if (isset($check->pass_val['err']['path_env'])) {
            	echo '<span class="field_error">*' . $check->pass_val['err']['path_env'] . '</span><br>';
            }
            ?>
            <input class="xtbl_value" type="text" name="path_env" size="100" value="<?php echo $options->GetValue('ExtraPathEnv');?>"></td>
    </tr>
    <tr class="xtbl_value">
            <td class="xtbl_label">Install Path Prefix</td>
            <td class="icon"></td>
            <td>
            <?
            if (isset($check->pass_val['err']['installPath'])) {
            	echo '<span class="field_error">*' . $check->pass_val['err']['installPath'] . '</span><br>';
            }
            ?>
            <input class="xtbl_value" type="text" name="installPath" size="100" value="<?php echo $options->GetValue('InstallPath');?>"></td>
    </tr>
    <tr class="xtbl_value">
            <td class="xtbl_label">Compiler Flags</td>
            <td class="icon">
				<img class="xtip-hover-compilerflags" src="/static/images/icons/help.png">
				<div id="xtip-note-compilerflags" class="snp-mouseoffset notedefault">
				<b>Compiler Options</b><hr size=1 color=black>You can add optimized compiler options here. Supported flags are CFLAGS, CXXFLAGS, CPPFLAGS, LDFLAGS.<br>
				 Example: CFLAGS='-O3 -msse2 -msse3 -msse4.1 -msse4.2 -msse4 -mavx' <br>
				 Syntax: Use space to separate different flags, use single quote instead of double-quotes for flag values<br>
				 </div>
            </td>
            <td>
            <?
            if (isset($check->pass_val['err']['compilerFlags'])) {
            	echo '<span class="field_error">*' . $check->pass_val['err']['compilerFlags'] . '</span><br>';
            }
            ?>
            <input class="xtbl_value" type="text" name="compilerFlags" size="100" value="<?php echo $options->GetValue('CompilerFlags');?>"></td>
    </tr>
    <tr class="xtbl_value">
            <td class="xtbl_label">Configure Parameters</td>
			<td class="icon">
				<img class="xtip-hover-phpconfigparam" src="/static/images/icons/help.png">
				<div id="xtip-note-phpconfigparam" class="snp-mouseoffset notedefault">
				<b>Configure Parameters</b><hr size=1 color=black>You can simply copy and paste the configure parameters
				 from the phpinfo() output of an existing working php build. The parameters that are Apache specific will be auto removed and
				 "--with-litespeed" will be auto appended when you click next step.<br><br>
				 </div>
			</td>


            <td>
            <?
            if (isset($check->pass_val['err']['configureParams'])) {
            	echo '<span class="field_error">*' . $check->pass_val['err']['configureParams'] . '</span><br>';
            }
            ?>
            <textarea name="configureParams" rows="6" cols="60" wrap="soft"><?php echo $options->GetValue('ConfigParam');?></textarea></td>
    </tr>
    <tr class="xtbl_value">
            <td class="xtbl_label">Add-on Modules</td>
            <td class="icon"></td>
    <td>
    	<?
    	$buf = '';
    	$checked = ' checked="checked"';
    	if ($supported['suhosin']) {
    		$buf .= '<input type="checkbox" name="addonSuhosin"';
    		if ($options->GetValue('AddOnSuhosin'))
    			$buf .= $checked;
    		$buf .= '> <a href="http://suhosin.org">Suhosin</a> (General Hardening) <br>';
    	}

        if ($supported['mailheader']) {
            $buf .= '<input type="checkbox" name="addonMailHeader"';
            if ($options->GetValue('AddOnMailHeader'))
                $buf .= $checked;
            $buf .= '> <a href="http://choon.net/php-mail-header.php">PHP Mail Header Patch</a> (Identifies Mail Source) <br>';
        }

        if ($supported['apc']) {
            $buf .= '<input type="checkbox" name="addonAPC"';
            if ($options->GetValue('AddOnAPC'))
                $buf .= $checked;
            $buf .= '> <a href="http://pecl.php.net/package/APC">APC</a> (Opcode Cache) V' . APC_VERSION . '<br>';
        }

        $buf .= '<input type="checkbox" name="addonXCache"';
        if ($options->GetValue('AddOnXCache'))
            $buf .= $checked;
        $buf .= '> <a href="http://xcache.lighttpd.net/">XCache</a>  (Opcode Cache) V' . XCACHE_VERSION;

        $buf .= '<br><input type="checkbox" name="addonMemCache"';
        if ($options->GetValue('AddOnMemCache'))
            $buf .= $checked;
        $buf .= '> <a href="http://pecl.php.net/package/memcache">memcache</a> (memcached extension) V' . MEMCACHE_VERSION . '<br>';

        if ($supported['opcache']) {
            $buf .= '<input type="checkbox" name="addonOPcache"';
            if ($options->GetValue('AddOnOPcache'))
                $buf .= $checked;
            $buf .= '> <a href="http://pecl.php.net/package/ZendOpcache">Zend OPcache</a> (Opcode Cache) V' . OPCACHE_VERSION . '<br>';
        }

    	$buf .= '<p class="field_note">Note: If you want to use a version not listed here, you can manually update the settings in /usr/local/lsws/admin/html/utility/build_php/buildconf.inc.php.</p>';

    	echo $buf;
    	?>

    </td>
    </tr>
</table>
<center>
	<input type="submit" name="back" value="Back to Step 1">
	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
	<input type="submit" name="buildsubmit" value="Build PHP <?=$php_version?>">
</center>

</form>

<br><br>


