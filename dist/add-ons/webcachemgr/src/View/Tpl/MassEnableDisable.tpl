<input type="hidden" name="act" />

<?php

use \Lsc\Wp\View\Model\MassEnableDisableViewModel as ViewModel;

$icon = $this->viewModel->getTplData(ViewModel::FLD_ICON);
$activeVer = $this->viewModel->getTplData(ViewModel::FLD_ACTIVE_VER);
$state = $this->viewModel->getTplData(ViewModel::FLD_STATE);

$d = array(
    'title' => 'Mass Enable/Disable LiteSpeed Cache',
    'icon' => $icon
);
$this->loadTplBlock('Title.tpl', $d);

if ( $state == ViewModel::ST_INSTALLS_DISCOVERED ):

?>

<p>
  The following operations will affect all WordPress installations not currently
  flagged.
</p>

<p>
  To exclude a WordPress installation from mass operations, flag it in
  <a href="?do=lscwp_manage" title="Go to Manage Cache Installations">
    Manage Cache Installations
  </a>
  .
</p>

<br />

<fieldset class="mass-box">
  <legend><b>Mass Enable</b></legend>

  <?php if ( $activeVer == false ): ?>

  <div>
    [Feature Disabled] No active LSCWP version set! Cannot Mass Enable.

    <br /><br />

  </div>

  <?php else: ?>

  <div>
    LiteSpeed Cache will be enabled for all WordPress installations not
    currently flagged. For WordPress installations that do not already have
    the plugin,
    <b>
      LiteSpeed Cache for WordPress <?php echo htmlspecialchars($activeVer); ?>
      will be installed.
    </b>
    This can be changed anytime in the
    <a href="?do=lscwpVersionManager" title="Go to Version Manager">Version Manager</a>.

    <br /><br />

    <button class="lsws-primary-btn" type="button"
            title="Mass Enable LiteSpeed Cache for all unflagged installations."
            onclick="javascript:actionset('enable');">
      Start
    </button>
  </div>

  <?php endif; ?>

</fieldset>

<br /><br />

<fieldset class="mass-box">
  <legend><b>Mass Disable</b></legend>
  <div>
    LiteSpeed Cache will be disabled for all WordPress installations not
    currently flagged.

    <br /><br />

    <button class="lsws-primary-btn" type="button"
            title="Mass Disable LiteSpeed Cache for all unflagged installations."
            onclick="javascript:actionset('disable');">
      Start
    </button>
  </div>
</fieldset>

<?php elseif ( $state == ViewModel::ST_NO_INSTALLS_DISCOVERED ): ?>

<div>
  <p>
    No WordPress installations discovered in the previous scan. If you have
    any newly installed WordPress installations, please go to
    <a href="?do=lscwp_manage" title="Go to Manage Cache Installations">
      Manage Cache Installations
    </a>
    and Re-scan/Discover New.
  </p>
</div>

<?php else: ?>

<div>
  <p>
    Please go to
    <a href="?do=lscwp_manage" title="Go to Manage Cache Installations">
      Manage Cache Installations
    </a>
    and click Scan to discover all active WordPress installations before
    attempting to Mass Enable/Disable Cache.
  </p>
</div>

<?php

endif;

$d = array(
    'back' => 'Back'
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);
