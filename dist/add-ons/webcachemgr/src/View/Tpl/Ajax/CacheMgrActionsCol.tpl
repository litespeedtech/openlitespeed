<?php

use Lsc\Wp\View\Model\Ajax\CacheMgrRowViewModel as ViewModel;

$listData = $this->viewModel->getTplData(ViewModel::FLD_LIST_DATA);

$classes = 'icon-btn';

foreach ( $listData as $path => $info ):
    $statusData = $info['statusData'];

?>

<td align="center">
  <span class="action-btns">
    <button type="button" value="<?php echo $path; ?>"
            class="<?php echo $classes; ?>"
            title="<?php echo $statusData['btn_title']; ?>"
            <?php echo ($statusData['onclick']) ? $statusData['onclick'] : ''; ?>
            <?php echo $statusData['btn_attributes']; ?>
            <?php echo $statusData['btn_state']; ?>
    >
      <?php echo $statusData['btn_content']; ?>
    </button>

    <?php if ( $statusData['sort'] == 'removed' ) : ?>

    <button type="button" value="<?php echo $path; ?>"
            class="<?php echo $classes; ?>"
    >
      <span class="inactive-refresh-btn"></span>
    </button>

    <?php else: ?>

    <button type="button" value="<?php echo $path; ?>"
            class="<?php echo $classes; ?>"
            title="Click to refresh status"
            onclick="javascript:lscwpRefreshSingle(this);"
            data-uk-tooltip
    >
      <span class="refresh_btn"></span>
    </button>

    <?php endif; ?>

  </span>
</td>

<?php endforeach;