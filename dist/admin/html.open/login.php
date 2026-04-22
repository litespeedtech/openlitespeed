<?php

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Product;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/view/inc/global.php';

$authorizer = CAuthorizer::singleton();
$msgType = 'info';
$productName = Product::GetInstance()->getProductName();

$is_https = (isset($_SERVER['HTTPS']) && ($_SERVER['HTTPS'] == 'on'));
if (!$is_https) {
    exit('Require https for admin panel.');
}
if (!$authorizer->ShowLogin($msg, $msgType)) {
	header('location:/index.php');
	exit();
}

?>
<!DOCTYPE html>
<html lang="en-us">
<?php
$no_main_header = true;
require_once __DIR__ . '/view/inc/header.php';
?>

<body class="lst-login-page">
<main class="lst-login-shell">
    <div class="lst-login-shell__viewport">
        <div class="lst-login-shell__column">
            <div class="lst-login-card">
                <form action="login.php" id="login" method="post" class="lst-auth-form lst-client-form" novalidate="novalidate">
                    <header class="lst-login-header">
                        <div class="lst-login-brand">
                            <img class="lst-login-logo" src="/res/img/product_logo.svg" alt="<?php echo UIBase::EscapeAttr($productName); ?>">
                        </div>
                        <div class="lst-login-wordmark" aria-hidden="true">WebAdmin Console</div>
                    </header>
                    <div class="lst-auth-fieldset">
<?php
$messageSectionClass = ($msg != '') ? 'lst-login-message-slot' : 'lst-login-message-slot lst-login-message-slot--empty';
echo '<section class="' . $messageSectionClass . '">';
if ($msg != '') {
 echo '<div class="lst-form-note lst-login-message lst-login-message--' . UIBase::EscapeAttr($msgType) . '"><span>' . UIBase::Escape($msg) . '</span></div>';
}
echo '</section>';
?>
                    <section>
                        <label class="lst-login-label" for="uid"><?php DMsg::EchoUIStr('l_username')?></label>
                        <div class="input"> <i class="icon-append lst-icon" data-lucide="user"></i>
                            <input type="text" id="uid" name="userid" tabindex="1" required autofocus="autofocus" autocomplete="username" placeholder="Username">
                        </div>
                    </section>

                    <section>
                        <label class="lst-login-label" for="pass"><?php DMsg::EchoUIStr('l_pass')?></label>
                        <div class="input lst-password-field" data-lst-password-field="1">
                            <button class="lst-password-toggle lst-login-password-toggle" type="button" rel="tooltip" data-placement="left" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::ALbl('btn_showpassword')); ?>" data-lst-password-toggle="1" data-lst-show-label="<?php echo UIBase::EscapeAttr(DMsg::ALbl('btn_showpassword')); ?>" data-lst-hide-label="<?php echo UIBase::EscapeAttr(DMsg::ALbl('btn_hidepassword')); ?>" aria-label="<?php echo UIBase::EscapeAttr(DMsg::ALbl('btn_showpassword')); ?>" aria-pressed="false">
                                <i class="lst-icon" data-lucide="eye" data-lst-password-show-icon="1" aria-hidden="true"></i>
                                <i class="lst-icon" data-lucide="eye-off" data-lst-password-hide-icon="1" aria-hidden="true" hidden></i>
                            </button>
                            <input type="password" id="pass" name="pass" data-lst-password-input="1" tabindex="2" required autocomplete="current-password" placeholder="Password">
                        </div>
                    </section>

                    </div>
                    <footer>
                        <button type="submit" class="lst-btn lst-btn--primary btn-login">
                            <?php DMsg::EchoUIStr('btn_signin')?>
                        </button>
                    </footer>
                </form>
            </div>
        </div>
    </div>
</main>
<?php require_once __DIR__ . '/view/inc/scripts.php'; ?>


</body>
</html>
