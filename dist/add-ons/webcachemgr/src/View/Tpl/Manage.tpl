<?php

use \Lsc\Wp\View\Model\ManageViewModel as ViewModel;

$iconDir = $this->viewModel->getTplData(ViewModel::FLD_ICON_DIR);
$scanBtnName = $this->viewModel->getTplData(ViewModel::FLD_SCAN_BTN_NAME);
$btnState = $this->viewModel->getTplData(ViewModel::FLD_BTN_STATE);
$activeVer = $this->viewModel->getTplData(ViewModel::FLD_ACTIVE_VER);
$listData = $this->viewModel->getTplData(ViewModel::FLD_LIST_DATA);
$countData = $this->viewModel->getTplData(ViewModel::FLD_COUNT_DATA);
$showList = $this->viewModel->getTplData(ViewModel::FLD_SHOW_LIST);
$warnMsgs = $this->viewModel->getTplData(ViewModel::FLD_WARN_MSGS);
$infoMsgs = $this->viewModel->getTplData(ViewModel::FLD_INFO_MSGS);
$errMsgs = $this->viewModel->getTplData(ViewModel::FLD_ERR_MSGS);
$succMsgs = $this->viewModel->getTplData(ViewModel::FLD_SUCC_MSGS);

?>

<div id="manager">

  <?php

  $d = array(
      'title' => 'Manage All LiteSpeed Cache for WordPress Installations',
      'icon' => "{$iconDir}/manageCacheInstallations.svg"
  );
  $this->loadTplBlock('Title.tpl', $d);

  ?>

  <div id="display-msgs">

    <?php

    if ( !empty($warnMsgs) ) {
        $d = array(
            'msgs' => $warnMsgs,
            'class' => 'msg-warn',
        );
        $this->loadTplBlock('DivMsgBox.tpl', $d);
    }

    if ( !empty($infoMsgs) ) {
        $d = array(
            'msgs' => $infoMsgs,
            'class' => 'msg-info',
        );
        $this->loadTplBlock('DivMsgBox.tpl', $d);
    }

    $errMsgCnt = count($errMsgs);
    $succMsgCnt = count($succMsgs);

    ?>

    <button class="accordion accordion-error" type="button"
            style="display: <?php echo ($errMsgCnt > 0) ? 'initial' : 'none'; ?>">
      Error Messages
      <span id ="errMsgCnt" class="badge errMsg-badge">
        <?php echo $errMsgCnt; ?>
      </span>
    </button>
    <div class="panel panel-error">

      <?php

      $d = array(
          'id' => 'errMsgs',
          'msgs' => $errMsgs,
          'class' => 'scrollable',
      );
      $this->loadTplBlock('DivMsgBox.tpl', $d);

      ?>

    </div>

    <button class="accordion accordion-success" type="button"
            style="display: <?php echo ($succMsgCnt > 0) ? 'initial' : 'none'; ?>">
      Success Messages
      <span id="succMsgCnt" class="badge succMsg-badge">
        <?php echo $succMsgCnt; ?>
      </span>
    </button>
    <div class="panel panel-success">

      <?php

      $d = array(
          'id' => 'succMsgs',
          'msgs' => $succMsgs,
          'class' => 'scrollable',
      );
      $this->loadTplBlock('DivMsgBox.tpl', $d);

      ?>

    </div>
  </div>

  <div align="left" >

    <?php

    $classes = '';
    $addClass = 'lsws-primary-btn';

    $d = array(
        'name' => 're-scan',
        'value' => $scanBtnName,
        'title' => 'Scan filesystem for WordPress installations',
        'confirm' => "{$scanBtnName} will scan your filesystem for WordPress installations. This may "
        . "take up to a few minutes to complete. {$scanBtnName} now?",
        'class' => "{$classes} {$addClass}"
    );
    $this->loadTplBlock('InputSubmitBtn.tpl', $d, true);

    if ( $btnState == 'disabled' ) {
        $addClass = 'disabled-btn';
    }

    $d = array(
        'name' => 'scan_more',
        'value' => 'Discover New',
        'title' => 'Discover new WordPress installations since the last scan',
        'confirm' => 'Discover new  WordPress installations since the last scan.This will not update '
                . 'information for existing installations. This may take up to a few minutes to '
                . 'complete. Continue?',
        'state' => $btnState,
        'class' => "{$classes} {$addClass}",
    );
    $this->loadTplBlock('InputSubmitBtn.tpl', $d, true);

    $d = array(
        'name' => 'refresh_status',
        'value' => 'Refresh Status',
        'title' => 'Check the cache status for all WordPress installations currently listed',
        'confirm' => 'Refresh Status will check the cache status for all WordPress installations currently '
                . 'listed. If you have many installations, this may take up to a few minutes to '
                . 'complete. Refresh Status now?',
        'state' => $btnState,
        'class' => "{$classes} {$addClass}",
    );
    $this->loadTplBlock('InputSubmitBtn.tpl', $d, true);

    $d = array(
        'name' => 'mass_unflag',
        'value' => 'Unflag All',
        'title' => 'Unflag all currently discovered installations',
        'confirm' => 'Unflag all currently discovered installations?',
        'state' => $btnState,
        'class' => "{$classes} {$addClass}",
    );
    $this->loadTplBlock('InputSubmitBtn.tpl', $d, true);

    ?>

  </div>

  <div align="left" class="pad-bottom-small">

    <?php

    $classes = 'lsws-secondary-btn';

    if ( $activeVer == false ) {
        $dTitle = '[Feature Disabled] No active LSCWP version set!';
        $dState = 'disabled';
    }
    else {
        $dTitle = 'Enable cache for all selected WordPress installations (Ignores Flag)';
        $dState = $btnState;
    }

    if ( $btnState == 'disabled' ) {
        $classes = 'disabled-btn';
        $dclasses = $classes;
    }
    else {
        $dclasses = ($dState != 'disabled') ? $classes : 'disabled-btn';
    }

    ob_start();

    ?>

    With Selected:
    <button type="button" name="enable_sel" value="Enable Selected"
            class="<?php echo $dclasses; ?>"
            title="<?php echo $dTitle; ?>"
            onclick="javascript:lscwpValidateSelectFormSubmit(this.name,
                this.value);"
            <?php echo $dState; ?>
    >
      Enable
    </button>
    <button type="button" name="disable_sel" value="Disable Selected"
            class="<?php echo $classes; ?>"
            title="Disable cache for all selected WordPress installations (Ignores Flag)"
            onclick="javascript:lscwpValidateSelectFormSubmit(this.name,
                this.value);"
            <?php echo $btnState; ?>
    >
      Disable
    </button>
    <button type="button" name="flag_sel" value="Flag Selected"
            class="<?php echo $classes; ?>"
            title="Flag all selected WordPress installations"
            onclick="javascript:lscwpValidateSelectFormSubmit(this.name,
                this.value);"
            <?php echo $btnState; ?>
    >
      Flag
    </button>
    <button type="button" name="unflag_sel" value="Unflag Selected"
            class="<?php echo $classes; ?>"
            title="Unflag all selected WordPress installations"
            onclick="javascript:lscwpValidateSelectFormSubmit(this.name,
                this.value);"
            <?php echo $btnState; ?>
    >
      Unflag
    </button>

    <?php

    $btn_row = ob_get_clean();

    echo $btn_row;

    ?>

  </div>

  <table class="plugin-ver">
    <tbody>
      <tr>
        <td align="right">
          LiteSpeed Cache Plugin Version:
          <a href="?do=lscwpVersionManager" title="Go to Version Manager">
            <?php echo ($activeVer) ? htmlspecialchars($activeVer) : 'Not Set'; ?>
          </a>
        </td>
      </tr>
    </tbody>
  </table>
<br/>
<div id="mask-container">
  <div id="hover-mask">
    <svg version="1.1" id="L9" xmlns="http://www.w3.org/2000/svg"
         xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
         viewBox="0 0 100 100" enable-background="new 0 0 0 0" xml:space="preserve">
        <path fill="#fff"
              d="M73,50c0-12.7-10.3-23-23-23S27,37.3,27,50 M30.9,50c0-10.5,8.5-19.1,19.1-19.1S69.1,39.5,69.1,50">
            <animateTransform
                attributeName="transform"
                attributeType="XML"
                type="rotate"
                dur="1s"
                from="0 50 50"
                to="360 50 50"
                repeatCount="indefinite" />
        </path>
    </svg>
  </div>

  <table id="lsws-data-table" class="datatable cachemgr hover">
    <thead>
      <tr>

        <?php

        $discoveredCnt = $countData[ViewModel::COUNT_DATA_INSTALLS];
        $enabledCnt = $countData[ViewModel::COUNT_DATA_ENABLED];
        $warnCnt = $countData[ViewModel::COUNT_DATA_WARN];
        $errCnt = $countData[ViewModel::COUNT_DATA_ERROR];
        $flagCnt = $countData[ViewModel::COUNT_DATA_FLAGGED];

        ?>
        <th width="20px"></th>
        <th>
          Discovered WordPress Installations
          <span id="total-badge" class="badge primary-badge" data-uk-tooltip
                title="<?php echo $discoveredCnt; ?> installations discovered">
            <?php echo $countData[ViewModel::COUNT_DATA_INSTALLS]; ?>
          </span>
          |
          <span id="enabled-badge" class="badge" data-uk-tooltip
                title="LSCWP is enabled for <?php echo $enabledCnt; ?> installations ">
            <?php echo $countData[ViewModel::COUNT_DATA_ENABLED]; ?>
          </span>
          <span id="warning-badge" class="badge" data-uk-tooltip
                title="LSCWP partially enabled for <?php echo $warnCnt; ?> installations">
            <?php echo $countData[ViewModel::COUNT_DATA_WARN]; ?>
          </span>
          <span id="error-badge" class="badge" data-uk-tooltip
                title="<?php echo $errCnt; ?> installations encountered a fatal error">
            <?php echo $countData[ViewModel::COUNT_DATA_ERROR]; ?>
          </span>
        </th>
        <th class="action-th">Actions</th>
        <th>
          Cache Status
        </th>
        <th>
          Flag
          <span id="flagged-badge" class="badge" data-uk-tooltip
                title="<?php echo $flagCnt; ?> installations flagged (excluded from mass operations)">
            <?php echo $countData[ViewModel::COUNT_DATA_FLAGGED]; ?>
          </span>
        </th>
      </tr>
    </thead>
    <tfoot>
      <tr>
        <th></th>
        <th></th>
        <th></th>
        <th></th>
        <th></th>
      </tr>
    </tfoot>
    <tbody>

      <?php

      if ( $showList ) :

          $classes = 'icon-btn';

          foreach ( $listData as $path => $info ):
              $statusData = $info['statusData'];
              $flagData = $info['flagData'];
              $siteUrl = $info['siteUrl'];
              $safePath = htmlspecialchars($path);

      ?>

      <tr>
        <td>
          <input type="checkbox" name="installations[]" value="<?php echo $path; ?>"
                 onclick="javascript:lscwpManageCheckboxSelect(this);" />
        </td>
        <td class="path-box">
          <?php echo htmlspecialchars($siteUrl); ?>
          <br />
          <small class="install-path"><?php echo $safePath; ?></small>
        </td>
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
            <button type="button" value="<?php echo $path; ?>"
                    class="<?php echo $classes; ?>"
                    title="Click to refresh status"
                    onclick="javascript:lscwpRefreshSingle(this);"
                    data-uk-tooltip
            >
              <span class="refresh_btn"></span>
            </button>
          </span>
        </td>
        <td align="center" data-search="<?php echo $statusData['sort']; ?>"
            data-sort="<?php echo $statusData['sort']; ?>">
          <?php echo $statusData['state']; ?>
          <a class="msg-alert" href="#display-msgs" data-uk-tooltip
             title="Click to go to messages">
          </a>
        </td>
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
      </tr>

      <?php

          endforeach;
      endif;

      ?>

    </tbody>
  </table>
  </div>

  <?php echo $btn_row; ?>

  <br /><br />
  <div>
    <small>
      *Flagging an installation will cause it to be excluded from
      Mass Enable/Disable operations.
    </small>
  </div>
  <br />
</div>
<script type="text/javascript">lswsInitDropdownBoxes();</script>
<script type="text/javascript">lswsInitDataTable();</script>