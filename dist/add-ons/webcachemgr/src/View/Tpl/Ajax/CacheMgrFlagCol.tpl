<?php

use Lsc\Wp\View\Model\Ajax\CacheMgrRowViewModel as ViewModel;

$listData = $this->viewModel->getTplData(ViewModel::FLD_LIST_DATA);

$classes = 'icon-btn';

foreach ( $listData as $path => $info ):
    $flagData = $info['flagData'];

?>

<td align="center"
    data-search="<?php echo ($flagData['sort'] == 'flagged') ? 'f' : 'u'; ?>"
    data-sort="<?php echo $flagData['sort']; ?>">
  <button type="button" value="<?php echo $path; ?>"
          class="<?php echo $classes; ?>"
          title="<?php echo $flagData['btn_title']; ?>"
          <?php echo $flagData['onclick']?>
          <?php echo $flagData['btn_attributes']; ?>
  >
    <?php echo $flagData['icon']; ?>
  </button>
</td>

<?php endforeach;