<?php

use \Lsc\Wp\View\Model\DataFileMsgViewModel as ViewModel;

$title = $this->viewModel->getTplData(ViewModel::FLD_TITLE);
$discover = $this->viewModel->getTplData(ViewModel::FLD_DISCOVER);

$d = array(
    'title' => $title
);
$this->loadTplBlock('Title.tpl', $d);

?>

<div>
  <p>
    Please go to
    <a href="?do=lscwp_manage" title="Go to Manage Cache Installations">
      Manage Cache Installations
    </a>
    and Scan to <?php echo $discover; ?> all active WordPress installations
    before continuing.
  </p>
</div>

<?php

$d = array(
    'back' => 'Back'
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);
