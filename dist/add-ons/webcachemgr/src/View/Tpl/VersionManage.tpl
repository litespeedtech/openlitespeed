<input type="hidden" name="act" />
<input type="hidden" name="version_num" />

<?php

use \Lsc\Wp\View\Model\VersionManageViewModel as ViewModel;

$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$activeVer = $this->viewModel->getTplData(ViewModel::FLD_ACTIVE_VER);
$verList = $this->viewModel->getTplData(ViewModel::FLD_VERSION_LIST);
$allowedList = $this->viewModel->getTplData(ViewModel::FLD_ALLOWED_VER_LIST);
$state = $this->viewModel->getTplData(ViewModel::FLD_STATE);
$errMsgs = $this->viewModel->getTplData(ViewModel::FLD_ERR_MSGS);

$d = array(
    'title' => 'LiteSpeed Cache Plugin Version Manager',
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

if ( !empty($errMsgs) ) {
    $d = array(
        'msgs' => $errMsgs,
        'class' => 'msg-error scrollable',
    );
    $this->loadTplBlock('DivMsgBox.tpl', $d);
}

$d = array(
    'title' => '<b>Set Active Version</b>'
);
$this->loadTplBlock('SectionTitle.tpl', $d);

?>

<?php if ( empty($allowedList) ): ?>

<div>
  <p>
    [Feature Disabled] Unable to find/retrieve LSCWP version list.
  </p>
</div>

<?php else: ?>

<div>
  <span class="hint">
    Sets the LiteSpeed Cache plugin version to be used in all future "Enable"
    operations.
  </span>
</div>
<br />
<table class="vermgr">
  <tbody>
    <tr>
      <td style="padding-top: 2px;">
        Current Active Version:
        <span style="color: #0066CC;">
          <?php echo ($activeVer) ? htmlspecialchars($activeVer) : 'Not Selected'; ?>
        </span>
      </td>
      <td>
        New Active Version
        <select id="lscwpActiveSelector">

          <?php

          foreach ( $allowedList as $ver ):
              $safeVer = htmlspecialchars($ver);

          ?>

          <option value ="<?php echo $safeVer; ?>">
            <?php echo $safeVer; ?>
          </option>

          <?php endforeach; ?>

        </select>
      </td>
      <td id="switchVerBtn">
        <button type="button" class="nowrap lsws-primary-btn"
                title="Switch LiteSpeed Cache plugin to the version selected"
                onclick="javascript:lscwpVermgr('switchTo');">
          Switch Version
        </button>
      </td>
    </tr>
  </tbody>
</table>

<?php endif; ?>

<br />

<?php

$d = array(
    'title' => '<b>Force Version Change For All Existing Installations</b>'
);
$this->loadTplBlock('SectionTitle.tpl', $d);

if ( $state == ViewModel::ST_SCAN_NEEDED ):

?>

<div>
  <p>
    Cache Management list could not be read. Please go to
    <a href="?do=lscwp_manage" title="Go to Manage Cache Installations">
      Manage Cache Installations
    </a>
    and Scan/Re-scan to discover active WordPress installations.
  </p>
</div>

<?php elseif ( $state == ViewModel::ST_NO_NON_ERROR_INSTALLS_DISCOVERED): ?>

<div>
  <p>
    No WordPress installations with a non-error status discovered in the
    previous scan (Installations with a Cache Status of "Error" are not
    counted). If you have any newly installed WordPress installations, please go
    to
    <a href="?do=lscwp_manage" title="Go to Manage Cache Installations">
      Manage Cache Installations
    </a>
    and Re-scan/Discover New.
  </p>
</div>

<?php elseif ( empty($verList) ): ?>

<div>
  <p>
    [Feature Disabled] Unable to find/retrieve LSCWP version list.
  </p>
</div>

<?php else: ?>

<div>
  <span class="hint">
    Upgrades active LiteSpeed Cache plugins matching the selection in the
    "From Version" list to the version chosen under "Upgrade To".
    <br />
    Flagged installations will be skipped.
  </span>
</div>

<br /><br />

<table id="lscwp-upgrade-mgr">
  <tbody>
    <tr>
      <td valign="top" style="margin-top: 9px;">From Version(s):</td>
      <td align="center">
        <table class="datatable nowrap">
          <thead>
            <tr>
              <th style="text-align: center;">
                <input type="checkbox" onClick="selectAll(this)" /> Select All
              </th>
            </tr>
          </thead>
          <tbody>

            <?php

            foreach ( $verList as $ver ):
                $safeVer = htmlspecialchars($ver);

            ?>

            <tr>
              <td>
                <input type="checkbox" name="selectedVers[]"
                       value="<?php echo $safeVer; ?>" />
                <?php echo $safeVer; ?>
              </td>
            </tr>

            <?php endforeach; ?>

          </tbody>
        </table>
      </td>
      <td valign="top">
        Upgrade To:
        <select id="lscwpUpgradeSelector">

          <?php

          foreach ( $allowedList as $ver ):
              $safeVer = htmlspecialchars($ver);
          ?>

          <option value ="<?php echo $safeVer; ?>">
            <?php echo $safeVer; ?>
          </option>

          <?php endforeach; ?>

        </select>
      </td>
      <td valign="top" align="center">
        <button type="button" title="Force version change for all matching selections"
                onclick="javascript:lscwpVermgr('upgradeTo');"
                class="upgrade-btn lsws-primary-btn">
          Upgrade
        </button>
      </td>
    </tr>
  </tbody>
</table>

<?php

endif;

$d = array(
    'back' => 'Back',
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);
