<?php

use \Lsc\Wp\View\Model\RefreshStatusProgressViewModel as ViewModel;

$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$installCount = $this->viewModel->getTplData(ViewModel::FLD_INSTALLS_COUNT);

$d = array(
    'title' => 'Refreshing Status of WordPress Installations...',
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

?>

<div id="progress-box" class="msg-box">
  Refreshing Status for <span id ="currIndex">0</span> out of
  <span id="totalCount"><?php echo $installCount; ?></span> ...
</div>

<?php

$d = array(
    'back' => 'Back',
    'backDo' => 'lscwp_manage',
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

<script type="text/javascript">lswsInitDropdownBoxes();</script>
<script type="text/javascript">lscwpRefreshStatusUpdate();</script>