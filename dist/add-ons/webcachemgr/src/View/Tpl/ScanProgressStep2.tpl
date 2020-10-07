<?php

/**
 * @since 1.13.3
 */

use \Lsc\Wp\View\Model\ScanProgressStepViewModel as ViewModel;

$title = $this->viewModel->getTplData(ViewModel::FLD_TITLE);
$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$totalCount = $this->viewModel->getTplData(ViewModel::FLD_TOTAL_COUNT);
$mgrStep = $this->viewModel->getTplData(ViewModel::FLD_MGR_STEP);

$d = array(
    'title' => $title,
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

?>

<div id="progress-box" class="msg-box">
    Scanning <span id="currIndex">0</span> out of
    <span id="totalCount"><?php echo $totalCount; ?></span> ...
</div>

<div>
    Currently attempting to refresh status for all WordPress installations found
    in the previous discovery step.
    <br />
    If performing a <b>Scan/Re-scan</b>, all installations will be found and
    added to the Cache Manager list.
    <br />
    If <b>Discovering New</b>, only installations belonging to users not already
    in the Cache Manager list will be discovered.
</div>
<br />

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
<script type="text/javascript">
    lscwpScanProgressStep1Update('<?php echo $mgrStep; ?>');
</script>