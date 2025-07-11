<?php

use Lsc\Wp\View\Model\FlagUnflagAllProgressViewModel as ViewModel;

$action       = $this->viewModel->getTplData(ViewModel::FLD_ACTION);
$icon         = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$installCount = $this->viewModel->getTplData(ViewModel::FLD_INSTALLS_COUNT);

$ucAction = ucfirst($action);

/** @noinspection SpellCheckingInspection */
$this->loadTplBlock(
  'Title.tpl',
  [
    'title' => "{$ucAction}ging WordPress Installations...",
    'icon'  => $icon
  ]
);

?>

<div id="progress-box" class="msg-box">
  <?php echo $ucAction; ?>ging <span id ="currIndex">0</span> out of
  <span id="totalCount"><?php echo $installCount; ?></span> ...
</div>

<?php

$this->loadTplBlock(
  'ButtonPanelBackNext.tpl',
  [ 'back' => 'Back', 'backDo' => 'lscwp_manage', 'visibility' => 'hidden' ]
);

?>

<button class="accordion accordion-error" type="button" style="display: none">
  Error Messages <span id ="errMsgCnt" class="badge errMsg-badge">0</span>
</button>
<div class="panel panel-error">

  <?php

  $this->loadTplBlock(
    'DivMsgBox.tpl',
    [ 'id' => 'errMsgs', 'class' => 'scrollable' ]
  );

  ?>

</div>

<button class="accordion accordion-success" type="button" style="display: none">
  Success Messages <span id="succMsgCnt" class="badge succMsg-badge">0</span>
</button>
<div class="panel panel-success">

  <?php

  $this->loadTplBlock(
    'DivMsgBox.tpl',
    [ 'id' => 'succMsgs', 'class' => 'scrollable' ]
  );

  ?>

</div>

<script type="text/javascript">
  lswsInitDropdownBoxes();
  lscwpFlagUnflagAllUpdate(<?php echo ($action == 'unflag') ? '4' : '5'; ?>);
</script>