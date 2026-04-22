<?php

use LSWebAdmin\I18n\DMsg;

?>
<script type="text/javascript">
    function lstHandleServiceRequestError(xhr) {
        var message = "";

        if (xhr && xhr.responseText) {
            message = String(xhr.responseText).replace(/\s+/g, " ").trim();
        }

        if (!message && xhr && xhr.statusText) {
            message = String(xhr.statusText).trim();
        }

        lstNotifyToast({
            title: "<?php DMsg::EchoUIStr('service_requestfailed') ?>",
            content: "<i class='lst-icon' data-lucide='triangle-alert'></i> <span>" + lstEscapeHtml(message) + "</span>",
            color: "#c0392b",
            timeout: 5000
        });
    }

    function lst_restart() {
        lstNotifyConfirm({
            title: "<i class='lst-icon lst-dialog-title-icon lst-dialog-title-icon--success' data-lucide='refresh-ccw-dot'></i> <span class='lst-dialog-title-text'><strong><?php DMsg::EchoUIStr('service_restartconfirm') ?></strong></span>",
            content: "<?php DMsg::EchoUIStr('service_willrefresh') ?>",
            buttons: '<?php echo '[' . DMsg::UIStr('btn_cancel') . '][' . DMsg::UIStr('btn_go') . ']' ; ?>'
        }, function (ButtonPressed) {
            if (ButtonPressed === "<?php DMsg::EchoUIStr('btn_go') ?>") {
                $.ajax({
                    type: "POST",
                    url: "view/serviceMgr.php",
                    data: {"act": "restart"},
                    beforeSend: function () {
                        lstNotifyToast({
                            title: "<?php DMsg::EchoUIStr('service_requesting') ?>",
                            content: "<i class='lst-icon' data-lucide='clock'></i> <i><?php DMsg::EchoUIStr('service_willrefresh') ?></i>",
                            color: "#659265",
                            timeout: 15000
                        });
                    },
                    success: function (data) {
                        location.reload(true);
                    },
                    error: function (xhr) {
                        lstHandleServiceRequestError(xhr);
                    }
                });
            }
        });
    }

    function lst_toggledebug() {
        lstNotifyConfirm({
            title: "<i class='lst-icon lst-dialog-title-icon lst-dialog-title-icon--danger' data-lucide='bug'></i> <span class='lst-dialog-title-text'><strong><?php DMsg::EchoUIStr('service_toggledebug') ?></strong></span>",
            content: "<?php DMsg::EchoUIStr('service_toggledebugmsg') ?>",
            buttons: '<?php echo '[' . DMsg::UIStr('btn_cancel') . '][' . DMsg::UIStr('btn_go') . ']' ; ?>'
        }, function (ButtonPressed) {
            if (ButtonPressed === "<?php DMsg::EchoUIStr('btn_go') ?>") {
                $.ajax({
                    type: "POST",
                    url: "view/serviceMgr.php",
                    data: {"act": "toggledebug"},
                    beforeSend: function () {
                        lstNotifyToast({
                            title: "<?php DMsg::EchoUIStr('service_requesting') ?>",
                            content: "<i class='lst-icon' data-lucide='clock'></i> <i><?php DMsg::EchoUIStr('service_willrefresh') ?></i>",
                            color: "#659265",
                            timeout: 2200
                        });
                    },
                    success: function (data) {
                        setTimeout(refreshLog, 2000);
                    },
                    error: function (xhr) {
                        lstHandleServiceRequestError(xhr);
                    }
                });
            }
        });
    }
</script>

<script src="/res/js/lst-app.js"></script>
<script src="/res/js/lucide.min.js"></script>
<script>
  if (typeof lucide !== 'undefined') {
    document.addEventListener('DOMContentLoaded', function() {
      lucide.createIcons();
    });
    // Also re-init on ajax load if applicable
    $(document).ajaxComplete(function() {
      if (typeof lstHideTooltip === 'function') { lstHideTooltip(); }
      lucide.createIcons();
    });
  }
</script>

<script type="text/javascript">
    // DO NOT REMOVE : GLOBAL FUNCTIONS!
    $(document).ready(function () {
        pageSetUp();
    });


</script>
