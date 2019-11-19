<?php

use \Lsc\Wp\View\Model\VersionChangeViewModel as ViewModel;

$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$installsCount = $this->viewModel->getTplData(ViewModel::FLD_INSTALLS_COUNT);
$verNum = $this->viewModel->getTplData(ViewModel::FLD_VER_NUM);

$d = array(
    'title' => 'Updating LiteSpeed Cache Plugins...',
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

?>

<div id="progress-box" class="msg-box">
  Attempting to upgrade <span id="currIndex">0</span> out of
  <span id="totalCount"><?php echo $installsCount; ?></span> ...
</div>

<div>
  Currently upgrading all known LiteSpeed Cache plugin installations to
  version <b><?php echo htmlspecialchars($verNum); ?></b>.
  Please be patient.
</div>
<br />

<?php

$d = array(
    'back' => 'Back',
    'backDo' => 'lscwpVersionManager',
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
<script type="text/javascript">lscwpVersionChangeUpdate();</script>