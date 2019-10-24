<?php

use Lsc\Wp\View\Model\Ajax\CacheMgrRowViewModel as ViewModel;

$listData = $this->viewModel->getTplData(ViewModel::FLD_LIST_DATA);

foreach ( $listData as $path => $info ):
    $statusData = $info['statusData'];

?>

<td align="center" data-search="<?php echo $statusData['sort']; ?>"
    data-sort="<?php echo $statusData['sort']; ?>">
  <?php echo $statusData['state']; ?>
  <a class="msg-alert" href="#display-msgs" data-uk-tooltip
    title="Click to go to messages">
  </a>
</td>

<?php endforeach;