<?php

use \Lsc\Wp\View\Model\MassEnableDisableProgressViewModel as ViewModel;

$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$action = $this->viewModel->getTplData(ViewModel::FLD_ACTION);
$installsCount = $this->viewModel->getTplData(ViewModel::FLD_INSTALLS_COUNT);
$activeVer = $this->viewModel->getTplData(ViewModel::FLD_ACTIVE_VER);

$actionUpper = ucfirst($action);

$d = array(
    'title' => 'Mass ' . substr($actionUpper, 0, -1) . 'ing All LiteSpeed Cache Plugins...',
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

if ( $action == 'enable' && $activeVer == false ):

?>

<div>
  No active LSCWP version set! Mass Enable aborted.
</div>

<?php

$d = array(
    'back' => 'OK'
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);

else:

?>

<div id="progress-box" class="msg-box">
  Attempting to <?php echo $action; ?> <span id="currIndex">0</span> out of
  <span id="totalCount"><?php echo $installsCount; ?></span> ...
</div>

<div>
  Currently attempting to <?php echo $action; ?> all LiteSpeed Cache
  plugin installations.

  <?php if ( $action == 'enable' ) : ?>

  If the LiteSpeed Cache Plugin is not installed, version
  <b><?php echo htmlspecialchars($activeVer); ?></b> will be used.

  <?php endif; ?>

  Please be patient.
</div>

<?php

$msgs = array(
    "<span id=\"bypassedCount\"><b>0</b></span> flagged/error WordPress installation(s) bypassed.",
    "LSCWP newly {$action}d for <span id=\"succCount\"><b>0</b></span> WordPress installation(s).",
    "LSCWP {$action} failed for <span id=\"failCount\" class=\"red\"><b>0</b></span> WordPress "
    . "installation(s)."
);

$d = array(
    'msgs' => $msgs,
    'class' => 'msg-info',
);
$this->loadTplBlock('DivMsgBox.tpl', $d);

$d = array(
    'back' => 'OK',
    'visibility' => 'hidden'
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);

?>

<button class="accordion accordion-error" type="button" style="display: none">
  Error Messages <span id ="errMsgCnt" class="badge errMsg-badge">0</span>
</button>
<div class="panel panel-error">

<?php

$d = array(
    'id' => 'errMsgs',
    'class' => 'scrollable',
);
$this->loadTplBlock('DivMsgBox.tpl', $d);

?>

</div>

<button class="accordion accordion-success" type="button" style="display: none">
  Success Messages <span id="succMsgCnt" class="badge succMsg-badge">0</span>
</button>
<div class="panel panel-success">

<?php

$d = array(
    'id' => 'succMsgs',
    'class' => 'scrollable',
    'title' => 'Success Messages:',
);
$this->loadTplBlock('DivMsgBox.tpl', $d);

?>
  
</div>

<script type="text/javascript">lswsInitDropdownBoxes();</script>
<script type="text/javascript">
  lscwpEnableDisableUpdate('<?php echo $action; ?>');
</script>

<?php endif;