<?php

use Lsc\Wp\View\Model\DashNotifierViewModel as ViewModel;

$iconDir = $this->viewModel->getTplData(ViewModel::FLD_ICON_DIR);
$discoveredCnt = $this->viewModel->getTplData(ViewModel::FLD_DISCOVERED_COUNT);
$rapMsgIds = $this->viewModel->getTplData(ViewModel::FLD_RAP_MSG_IDS);
$bamMsgIds = $this->viewModel->getTplData(ViewModel::FLD_BAM_MSG_IDS);

/**
 * Close usual "lswsform" to allow the following page to have multiple forms
 * based on selected tab.
 */
echo '</form>';

?>



<div id="heading">
  <h1 class="uk-margin-top-remove">

    <?php if ( $iconDir != '' ) : ?>

    <span>
      <img src="<?php echo $iconDir; ?>/wpNotifier.svg" alt="wp_dash_notifier_icon" />
    </span>

    <?php endif; ?>

    WordPress Dashboard Notification
  </h1>
  <p class="uk-width-medium-2-3 uk-width-small-1-1 uk-text-dark"
     style="font-size: 15px; margin-left: 0px;">
    This tool deploys and activates the
    <a href="https://wordpress.org/plugins/dash-notifier/" target='_blank'>WP Dash Notifier</a>
    WordPress plugin on all of the server's WordPress installations.
  </p>
</div>

<div id="display-msgs">
  <button class="accordion accordion-error" type="button" style="display: none">
    Error Messages
    <span id ="errMsgCnt" class="badge errMsg-badge">0</span>
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

  <button class="accordion accordion-success" type="button" style="display: none">
    Success Messages
    <span id="succMsgCnt" class="badge succMsg-badge">0</span>
  </button>

  <div class="panel panel-success">

    <?php

    $d = array(
        'id' => 'succMsgs',
        'class' => 'scrollable',
    );
    $this->loadTplBlock('DivMsgBox.tpl', $d);

    ?>

  </div>
</div>

<ul class="uk-tab uk-tab-grid" data-uk-tab="{connect:'#notify'}">
  <li>
    <a href="#">
      <i class="fa fa-puzzle-piece fa-1x uk-margin-small-right"></i>Recommend A
      Plugin
    </a>
  </li>
  <li>
    <a href="#">
      <i class="fa fa-paper-plane fa-1x uk-margin-small-right"></i>Broadcast A
      Message
    </a>
  </li>
</ul>

<ul id="notify" class="uk-switcher uk-margin">
  <li>
    <form name="rap-form" accept-charset="utf-8">
      <input type="hidden" name="step" value="1"/>
      <input type="hidden" name="do" value="dash_notifier"/>
      <div class="uk-block uk-block-default uk-panel-space
             uk-padding-top-remove uk-padding-bottom-remove">
        <div>
          <div class="uk-margin-bottom uk-margin-top uk-text-dark">
            Broadcast a notification message recommending a particular plugin,
            complete with an install/activate button for that plugin.
          </div>
        </div>

        <button class="accordion uk-margin-bottom button-primary" type="button">
            <i class="fa fa-info-circle fa-1x uk-margin-small-right
                 uk-text-primary"></i>
            Recommending A Plugin
        </button>
          <div class="panel uk-accordion-content-background">
            <div class="uk-margin uk-margin-top">
              <p>
                On this page, a plugin can be recommended to all
                <a href="?do=lscwp_manage">discovered WordPress installations</a>
                by providing both the plugin's slug and a dashboard message.
                The following illustrates possible dashboard messages shown to
                WordPress users when recommending a plugin with plugin slug
                'litespeed-cache' with a simple plain text message.
              </p>
              <ol>
                <li class="uk-margin-top">
                  <span>
                    User does not have the 'litespeed-cache' plugin installed
                  </span>
                  <figure class="uk-margin-small uk-margin-small-top">
                    <img src="<?php echo $iconDir; ?>/../images/dash-notify-install.png"
                         alt="recommend_new_plugin_img" class="uk-border-rounded"/>
                  </figure>
                </li>
                <li class="uk-margin-top">
                  <span>
                    User has 'litespeed-cache' plugin installed, but
                    deactivated
                  </span>
                  <figure class="uk-margin-small uk-margin-small-top">
                    <img src="<?php echo $iconDir; ?>/../images/dash-notify-activate.png"
                         alt="recommend_disabled_plugin_img"
                         class="uk-border-rounded" />
                  </figure>
                </li>
                <li class="uk-margin-top">
                  <span>
                    User has 'litespeed-cache' plugin installed and active
                    <br />
                    * No message will be displayed
                  </span>
                </li>
              </ol>
              <h3>Other Info</h3>
              <ul class="uk-list-space">
                <li>
                  WordPress users who click the "Dismiss" button will not be
                  re-notified if the provided dashboard message and plugin slug
                  match those of the dismissed message.
                </li>
                <li>
                  WordPress users who click the "Never Notify Me Again" button
                 will have the Dash Notifier plugin uninstalled and a
                 '.dash_notifier_bypass' file created in their installation's
                 root directory. These users will not be notified of any
                 further messages while this file exists.
                </li>
              </ul>
            </div>
          </div>

        <ul class="uk-grid uk-grid-width-large-1-2 uk-grid-width-medium-1-1
              uk-grid-small">
          <li class="uk-panel uk-panel-space">
            <div class="">
              <span>Plugin Slug</span>
              <span class="far fa-question-circle uk-margin-small-right
                      uk-text-muted"
                    style="font-size: 14px;"
                    title="A plugin's unique identifier. This can be found by finding the plugin's page on wordpress.org/plugins and taking the last part of the URL. For example, the URL for the LiteSpeed Cache plugin for WordPress is wordpress.org/plugins/litespeed-cache, making the plugin slug 'litespeed-cache'."
                    data-uk-tooltip></span>
              <input id="pluginSlug" class="uk-margin-right" type="text"
                     name="notify_slug" size="30" value=""/>
              <button class="nowrap lsws-muted-btn" type="button" name="verify_slug"
                      value="Verify Plugin Slug" title="Verify provided WordPress plugin slug"
                      onclick="javascript:dashVerifySlug(
                                  document.getElementById('pluginSlug').value);">
                Verify
              </button>
              <br />
              <span id="verify-output" class="uk-text-small"></span>
            </div>

            <div>
              <div>
                <span>WordPress Dashboard Message</span>
                <span class="far fa-question-circle uk-text-muted"
                  style="font-size: 14px;"
                  title="Message to be displayed as a Dash Notifier message. Both plain text and HTML are allowed. Newlines will be ignored in plain text."
                  data-uk-tooltip></span>

                <button type="button" class="uk-margin-left uk-button-link"
                        name="clear_msg" value="Clear Current Message"
                        title="Clear current WordPress Dashboard Message"
                        onclick="javascript:clearDashMsg('rap');" data-uk-tooltip>
                  <i class="fa fa-times"
                     style="color: #FF5C85; font-size: 18px;"></i>
                </button>
              </div>
              <div>
                <textarea id="rap-dashMsg" class="uk-margin-small-top" rows="10"
                          name="notify_msg"
                          style="width: 90%; resize: vertical;"></textarea>
              </div>
            </div>
          </li>

          <li class="uk-panel uk-panel-space">
            <div class="uk-margin-bottom">
              <div class="uk-margin-small-right">
              <span>Stored Messages</span>
              <span class="far fa-question-circle uk-text-muted"
                    style="font-size: 14px;"
                    title="Load or delete previously saved dashboard message + plugin slug combinations"
                    data-uk-tooltip></span>
              </div>
              <div>
                <select id="rap-msgIds"
                        style="min-width: 200px;border-color: #bbb;
                          border-radius: 5px;">

                  <?php

                  foreach ( $rapMsgIds as $id ):
                      $safeId = htmlspecialchars($id);

                  ?>

                  <option value ="<?php echo $safeId; ?>">
                    <?php echo $safeId; ?>
                  </option>

                  <?php endforeach; ?>

                </select>
                <span id="savedMsgsBtnsRap">
                  <button type="button" class="nowrap lsws-muted-btn"
                          title="Load the selected version"
                          onclick="javascript:loadSavedDashMsg('rap');">
                    Load
                  </button>
                  <button type="button" class="nowrap lsws-muted-btn"
                          title="Delete the selected version"
                          onclick="javascript:deleteSavedDashMsg('rap');">
                    Delete
                  </button>
                </span>
              </div>
            </div>

            <div class="uk-margin-large-bottom">
                <div class="uk-margin-small-right">
                  <span>Save As</span>
                  <span class="far fa-question-circle uk-text-muted"
                        style="font-size: 14px;"
                        title="Save the entered Dashbaord Message and Plugin Slug combination. Maximum of 50 characters (a-zA-Z0-9_-)."
                        data-uk-tooltip></span>
                </div>
                <div>
                  <input id="rap-saveKey" type="text"
                         name="save_key" value=""/>
                  <button class="nowrap lsws-muted-btn" type="button" name="save_msg"
                          value="Save Message As"
                          title="Save current message with the provided ID"
                          onclick="javascript:saveDashMsg(
                                      document.getElementById('rap-saveKey').value,
                                      'rap');">
                    Save
                  </button>
                </div>
            </div>
          </li>
        </ul>

        <button class="accordion uk-margin-bottom button-primary" type="button">
          <span class="uk-margin-small-left">Testing / Preview</span>
        </button>
        <div class="panel uk-padding-top uk-accordion-content-background"
             style="border-radius: 0 0 5px 5px;">
          <div class="uk-margin uk-margin-top">
            <span>
              Use this option to test enabling/disabling notifications on a
              single WordPress installation by providing it's path.
            </span>
            <br /><br />
            <span class="uk-margin-small-right">
              Specify a WordPress installation path
            </span>
            <input class="uk-margin-right" id="rap-wpPath" type="text" name="wpPath"
                   size="40" value=""/>
            <span id="rap-notifySingleBtn">
              <button class="lsws-primary-btn" type="button" name="notify_single"
                      value="Notify Installation" title="Notify provided WordPress installation"
                      onclick="javascript:dashNotifySingle(
                                  document.getElementById('rap-wpPath').value,
                                  document.getElementById('rap-dashMsg').value
                                  .replace(/^\s+|\s+$/g, ''), 'rap');">
                Deploy / Notify
              </button>
            </span>
            <span id="rap-disableSingleBtn">
              <button class="lsws-primary-btn" type="button" name="disable_single"
                      value="Disable for Installation"
                      title="Disable Dash Notifier for provided WordPress installation"
                      onclick="javascript:dashDisableSingle(
                                  document.getElementById('rap-wpPath').value,
                                  'rap');">
                Remove
              </button>
            </span>
            <br />
          </div>
        </div>

        <div class="uk-block uk-margin-top uk-padding-top uk-padding-bottom">
          <span>
            Discovered WordPress installations
            <a href="?do=lscwp_manage">
              (<b><?php echo $discoveredCnt; ?></b> Discovered
            )</a>
          </span>
          <br /><br />
          <button type="button" name="mass-notify" value="Deploy / Notify All"
                  class="lsws-primary-btn"
                  title="Notify all WordPress installations discovered in 'Manage Cache Installations' using the Dashboard Notifier plugin."
                  onclick="javascript:dashMassActionSubmit(this.name,
                      this.value, 'rap');"
          >
            Mass Deploy / Notify
          </button>

          <button type="button" name="mass-disable-dash-notifier" value="Remove All"
                  class="lsws-primary-btn"
                  title="Disable and remove Dashboard Notifier Wordpress plugin from all WordPress installations discovered in 'Manage Cache Installations'."
                  onclick="javascript:dashMassActionSubmit(this.name,
                      this.value, 'rap');"
          >
            Mass Remove
          </button>
        </div>
      </div>
    </form>
    <hr />
  </li>

  <li>
    <form name="bam-form" accept-charset="utf-8">
      <input type="hidden" name="step" value="1"/>
      <input type="hidden" name="do" value="dash_notifier"/>
      <div class="uk-block uk-block-default uk-panel-space
             uk-padding-top-remove uk-padding-bottom-remove">
        <div>
          <div class="uk-margin-bottom uk-margin-top uk-text-dark">
            Broadcast simple informational messages.
          </div>
        </div>

        <button class="accordion uk-margin-bottom button-primary" type="button">
            <i class="fa fa-info-circle fa-1x uk-margin-small-right
                 uk-text-primary"></i>
              Broadcasting A Message
        </button>
        <div class="panel uk-accordion-content-background">
          <div class="uk-margin uk-margin-top">
            <p>
              On this page, a message can be sent to all
              <a href="?do=lscwp_manage">discovered WordPress installations</a>
              by providing a dashboard message. The following illustrates
              the dashboard messages shown to WordPress users when sending a
              simple plain text message.
            </p>
            <ol>
              <li class="uk-margin-top">
               <figure class="uk-margin-small uk-margin-small-top">
                <img src="<?php echo $iconDir; ?>/../images/dash-notify-simple.png"
                     alt="send_a_message_img" class="uk-border-rounded"/>
               </figure>
              </li>
            </ol>

            <h3>Other Info</h3>
            <ul class="uk-list-space">
              <li>
                WordPress users who click the "Dismiss" button will not be
                re-notified if the provided dashboard message matches that
                of the last dismissed message.
              </li>
              <li>
               WordPress users who click the "Never Notify Me Again" button
               will have the Dash Notifier plugin uninstalled and a
               '.dash_notifier_bypass' file created in their installation's
               root directory. These users will not be notified of any
               further messages while this file exists.
              </li>
            </ul>
          </div>
        </div>

        <ul class="uk-grid uk-grid-width-large-1-2 uk-grid-width-medium-1-1
              uk-grid-small">
          <li class="uk-panel uk-panel-space">
            <div>
              <div>
                <span>WordPress Dashboard Message</span>
                <span class="far fa-question-circle uk-text-muted"
                      style="font-size: 14px;"
                      title="Message to be displayed as a Dash Notifier message. Both plain text and HTML are allowed. Newlines will be ignored in plain text."
                      data-uk-tooltip></span>
                <button class="uk-margin-left uk-button-link" type="button"
                        name="clear_msg" value="Clear Current Message"
                        title="Clear current WordPress Dashboard Message"
                        onclick="javascript:clearDashMsg('bam');" data-uk-tooltip>
                  <i class="fa fa-times"
                     style="color: #FF5C85; font-size: 18px;"></i>
                </button>
              </div>
              <div>
                <textarea id="bam-dashMsg" class="uk-margin-small-top" rows="10"
                          name="notify_msg"
                          style="width: 90%; resize: vertical;"></textarea>
              </div>
            </div>
          </li>

          <li class="uk-panel uk-panel-space">
            <div class="uk-margin-bottom">
              <div class="uk-margin-small-right">
                <span>Stored Messages</span>
                <span class="far fa-question-circle uk-text-muted"
                      style="font-size: 14px;"
                      title="Load or delete previously saved dashboard message + plugin slug combinations"
                      data-uk-tooltip></span>
              </div>
              <div>
                <select id="bam-msgIds"
                        style="min-width: 200px; border-color: #bbb;
                          border-radius: 5px;">

                  <?php

                  foreach ( $bamMsgIds as $id ):
                      $safeId = htmlspecialchars($id);

                  ?>

                  <option value ="<?php echo $safeId; ?>">
                    <?php echo $safeId; ?>
                  </option>

                  <?php endforeach; ?>

                </select>
                <span id="savedMsgsBtnsBam">
                  <button type="button" class="nowrap lsws-muted-btn"
                          title="Load the selected version"
                          onclick="javascript:loadSavedDashMsg('bam');">
                    Load
                  </button>
                  <button type="button" class="nowrap lsws-muted-btn"
                          title="Delete the selected version"
                          onclick="javascript:deleteSavedDashMsg('bam');">
                    Delete
                  </button>
                </span>
              </div>
            </div>

            <div>
              <div class="uk-margin-small-right">
                <span>Save As</span>
                <span class="far fa-question-circle uk-text-muted"
                      style="font-size: 14px;"
                      title="Save the entered Dashbaord Message and Plugin Slug combination. Maximum of 50 characters (a-zA-Z0-9_-)."
                      data-uk-tooltip></span>
              </div>
              <div>
                <input id="bam-saveKey" type="text" name="save_key" value=""/>
                <button class="nowrap lsws-muted-btn" type="button" name="save_msg"
                        value="Save Message As"
                        title="Save current message with the provided ID"
                        onclick="javascript:saveDashMsg(
                                    document.getElementById('bam-saveKey').value,
                                    'bam');">
                  Save
                </button>
              </div>
            </div>
          </li>
        </ul>

        <button class="accordion uk-margin-bottom button-primary" type="button">
          <span class="uk-margin-small-left">Testing / Preview</span>
        </button>
        <div class="panel uk-padding-top uk-accordion-content-background"
             style="border-radius: 0 0 5px 5px;">
          <div class="uk-margin uk-margin-top">
            <span>
              Use this option to test enabling/disabling notifications on a
              single WordPress installation by providing it's path.
            </span>
            <br /><br />
            <span class="uk-margin-small-right">
              Specify a WordPress installation path
            </span>
            <input class="uk-margin-right" id="bam-wpPath" type="text" name="wpPath"
                   size="40" value=""/>
            <span id="bam-notifySingleBtn">
              <button class="lsws-primary-btn" type="button" name="notify_single"
                      value="Notify Installation" title="Notify provided WordPress installation"
                      onclick="javascript:dashNotifySingle(
                                  document.getElementById('bam-wpPath').value,
                                  document.getElementById('bam-dashMsg').value
                                  .replace(/^\s+|\s+$/g, ''), 'bam');">
                Deploy / Notify
              </button>
            </span>
            <span id="bam-disableSingleBtn">
              <button class="lsws-primary-btn" type="button" name="disable_single"
                      value="Disable for Installation"
                      title="Disable Dash Notifier for provided WordPress installation"
                      onclick="javascript:dashDisableSingle(
                                  document.getElementById('bam-wpPath').value,
                                  'bam');">
                Remove
              </button>
            </span>
            <br />
          </div>
        </div>

        <div class="uk-block uk-margin-top uk-padding-top uk-padding-bottom">
          <span>
            Discovered WordPress installations
            <a href="?do=lscwp_manage">
              (<b><?php echo $discoveredCnt; ?></b> Discovered)
            </a>
          </span>
          <br /><br />
          <button type="button" name="mass-notify" value="Deploy / Notify All"
                  class="lsws-primary-btn"
                  title="Notify all WordPress installations discovered in 'Manage Cache Installations' using the Dashboard Notifier plugin."
                  onclick="javascript:dashMassActionSubmit(this.name,
                      this.value, 'bam');"
          >
            Mass Deploy / Notify
          </button>

          <button type="button" name="mass-disable-dash-notifier" value="Remove All"
                  class="lsws-primary-btn"
                  title="Disable and remove Dashboard Notifier Wordpress plugin from all WordPress installations discovered in 'Manage Cache Installations'."
                  onclick="javascript:dashMassActionSubmit(this.name,
                      this.value, 'bam');"
          >
            Mass Remove
          </button>
          <br />
        </div>
      </div>
    </form>
    <hr />
  </li>
</ul>

<?php

$d = array(
    'back' => 'Back'
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);

?>

<script type="text/javascript">lswsInitDropdownBoxes();</script>
