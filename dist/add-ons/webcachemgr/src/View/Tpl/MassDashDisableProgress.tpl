<?php

use \Lsc\Wp\View\Model\MassDashDisableProgressViewModel as ViewModel;

$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$installsCount = $this->viewModel->getTplData(ViewModel::FLD_INSTALLS_COUNT);

$d = array(
    'title' => 'Mass Notifying All Discovered WordPress Installations...',
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

?>

<div id="progress-box" class="msg-box">
  Attempting to disable Dash Notifier for <span id="currIndex">0</span> out of
  <span id="totalCount"><?php echo $installsCount; ?></span> ...
</div>

<div>
  Currently attempting to disable Dash Notifier for all discovered WordPress
  installations. Installations detected with "Error" status will be bypassed.

  Please be patient.
</div>

<?php

$msgs = array(
    "<span id=\"bypassedCount\"><b>0</b></span> WordPress installation(s) bypassed.",
    "Dash Notifier disabled for <span id=\"succCount\"><b>0</b></span> WordPress installation(s).",
    "Dash Notifier failed to disable for <span id=\"failCount\" class=\"red\"><b>0</b></span> WordPress "
            . "installation(s)."
);

$d = array(
    'msgs' => $msgs,
    'class' => 'msg-info',
);
$this->loadTplBlock('DivMsgBox.tpl', $d);

$d = array(
    'back' => 'OK',
    'backDo' => 'dash_notifier',
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
<script type="text/javascript">dashMassDisableUpdate();</script>
