<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Product;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\UIBase;

if (!$no_main_header) { ?>
<!DOCTYPE html>
<html lang="en-us">
<?php } ?>
	<head>
<?php
$product = Product::GetInstance();
$productName = $product->getProductName();
$consoleName = $product->getWebAdminConsoleName();
?>
		<meta charset="utf-8">
		<title><?php echo UIBase::Escape($consoleName); ?></title>
		<meta name="description" content="<?php echo UIBase::EscapeAttr($consoleName); ?>">
		<meta name="author" content="LiteSpeed Technologies, Inc.">

		<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">

		<script type="text/javascript">
			(function () {
				var theme = 'light';
				var shellNavMode = 'expanded';
				try {
					if (window.localStorage) {
						var storedTheme = window.localStorage.getItem('lst.theme');
						if (storedTheme === 'light' || storedTheme === 'dark') {
							theme = storedTheme;
						}
					}
					if (window.sessionStorage) {
						var storedShellNavMode = window.sessionStorage.getItem('lst.shell.nav');
						if (storedShellNavMode === 'expanded' || storedShellNavMode === 'minified' || storedShellNavMode === 'hidden') {
							shellNavMode = storedShellNavMode;
						}
					}

					if (theme === 'light' && window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
						theme = 'dark';
					}
				} catch (err) {
				}

				document.documentElement.setAttribute('data-lst-theme', theme);
				document.documentElement.style.colorScheme = theme;
				window.__lstShellNavMode = shellNavMode;
			})();
		</script>

		<link rel="shortcut icon" href="/res/img/favicon/favicon.ico" type="image/x-icon">
		<link rel="icon" href="/res/img/favicon/favicon.ico" type="image/x-icon">
		
		<link rel="stylesheet" type="text/css" media="screen" href="/res/css/googlefonts.css">
		<link rel="stylesheet" type="text/css" media="screen" href="/res/css/lst-theme.css">
		<link rel="stylesheet" type="text/css" media="screen" href="/res/css/lst-product-accent.css">
		<link rel="stylesheet" type="text/css" media="screen" href="/res/css/lst-components.css">
		<link rel="stylesheet" type="text/css" media="screen" href="/res/css/lst-shell.css">
<?php if ($no_main_header) { ?>
		<link rel="stylesheet" type="text/css" media="screen" href="/res/css/lst-page-login.css">
<?php } else {
		$lstPageStyles = [];
		if (isset($view)) {
			if ($view === 'dashboard') {
				$lstPageStyles[] = '/res/css/lst-page-dashboard.css';
				$lstPageStyles[] = '/res/css/lst-page-tools.css';
			} elseif (UI::UsesToolPageStyle($view)) {
				$lstPageStyles[] = '/res/css/lst-page-tools.css';
			}
		}
		foreach ($lstPageStyles as $lstPageStyleHref) {
?>
		<link rel="stylesheet" type="text/css" media="screen" href="<?php echo UIBase::EscapeAttr($lstPageStyleHref); ?>">
<?php
		}
} ?>
		<meta name="mobile-web-app-capable" content="yes">
		<meta name="apple-mobile-web-app-capable" content="yes">
		<meta name="apple-mobile-web-app-status-bar-style" content="black">
		<meta name="robots" content="noindex">

		<script src="/res/js/libs/jquery-4.0.0.min.js"></script>
			<script type="text/javascript">
			(function () {
				var lstPasswordShowLabel = <?php echo json_encode(DMsg::ALbl('btn_showpassword')); ?>;
				var lstPasswordHideLabel = <?php echo json_encode(DMsg::ALbl('btn_hidepassword')); ?>;

				window.lstUiLabels = window.lstUiLabels || {};
				window.lstUiLabels.cancel = <?php echo json_encode(DMsg::UIStr('btn_cancel')); ?>;
				window.lstUiLabels.go = <?php echo json_encode(DMsg::UIStr('btn_go')); ?>;
				window.lstUiLabels.signOut = <?php echo json_encode(DMsg::UIStr('note_signout')); ?>;
				window.lstUiLabels.signOutOf = <?php echo json_encode(DMsg::UIStr('note_signoutof')); ?>;
				window.lstUiLabels.rows = <?php echo json_encode(DMsg::UIStr('note_rows')); ?>;
				window.lstUiLabels.editOrder = <?php echo json_encode(DMsg::UIStr('btn_edit') . ' ' . DMsg::ALbl('l_order')); ?>;
				window.lstUiLabels.saveOrder = <?php echo json_encode(DMsg::UIStr('btn_save') . ' ' . DMsg::ALbl('l_order')); ?>;
				window.lstUiLabels.dismissNotification = <?php echo json_encode(DMsg::UIStr('note_dismissnotification')); ?>;
				window.lstUiLabels.dismissHelp = <?php echo json_encode(DMsg::UIStr('note_dismisshelp')); ?>;
				window.lstUiLabels.moveItemUp = <?php echo json_encode(DMsg::UIStr('note_moveitemup')); ?>;
				window.lstUiLabels.moveItemDown = <?php echo json_encode(DMsg::UIStr('note_moveitemdown')); ?>;

				function lstSetPasswordToggleState(button, isVisible) {
					var showLabel = button.getAttribute('data-lst-show-label') || lstPasswordShowLabel;
					var hideLabel = button.getAttribute('data-lst-hide-label') || lstPasswordHideLabel;
					var label = isVisible ? hideLabel : showLabel;
					var showIcon = button.querySelector('[data-lst-password-show-icon]');
					var hideIcon = button.querySelector('[data-lst-password-hide-icon]');

					button.setAttribute('aria-pressed', isVisible ? 'true' : 'false');
					button.setAttribute('aria-label', label);
					button.setAttribute('data-original-title', label);

					if (showIcon) {
						showIcon.hidden = isVisible;
					}
					if (hideIcon) {
						hideIcon.hidden = !isVisible;
					}
				}

				document.addEventListener('click', function (event) {
					var button = event.target.closest('[data-lst-password-toggle]');
					var field;
					var input;
					var selectionStart;
					var selectionEnd;
					var shouldReveal;

					if (!button) {
						return;
					}

					field = button.closest('[data-lst-password-field]');
					input = field ? field.querySelector('[data-lst-password-input]') : null;

					if (!input || button.disabled || input.disabled) {
						return;
					}

					event.preventDefault();

					selectionStart = (typeof input.selectionStart === 'number') ? input.selectionStart : null;
					selectionEnd = (typeof input.selectionEnd === 'number') ? input.selectionEnd : null;
					shouldReveal = input.getAttribute('type') === 'password';

					try {
						input.setAttribute('type', shouldReveal ? 'text' : 'password');
					} catch (err) {
						return;
					}

					lstSetPasswordToggleState(button, shouldReveal);

					if (typeof input.focus === 'function') {
						input.focus();
					}
					if (selectionStart !== null && selectionEnd !== null && typeof input.setSelectionRange === 'function') {
						input.setSelectionRange(selectionStart, selectionEnd);
					}
				});
			})();
		</script>
	</head>
<?php if ($no_main_header) return; ?>
	<body class="lst-app-shell"
	      data-back-to-top-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_top')); ?>">
	<script type="text/javascript">
		(function () {
			var body = document.body;
			var shellNavMode = window.__lstShellNavMode || 'expanded';

			if (!body) {
				return;
			}

			if (shellNavMode === 'minified') {
				body.classList.add('minified');
			}
			else if (shellNavMode === 'hidden') {
				body.classList.add('hidden-menu');
			}
		})();
	</script>

		<header id="header" class="lst-topbar">
			<div class="lst-topbar-intro">
				<div class="lst-header-control lst-mobile-menu-control">
					<span><a href="#" class="lst-mobile-menu-trigger" data-action="toggleMenu" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_navigation')); ?>" aria-controls="left-panel" aria-expanded="false"><i class="lst-icon" data-lucide="menu"></i></a></span>
				</div>
				<div id="logo-group" class="lst-brand">
	                            <span id="logo" class="lst-brand-logo">
	                                <img src="/res/img/product_logo.svg" alt="<?php echo UIBase::EscapeAttr($productName); ?>" width="200" height="50">
	                            </span>
				</div>


				<div class="project-context lst-hide-phone lst-version-bar">

			<?php
			$prod = Product::GetInstance();
			$version = $prod->getVersion();
			$host_name = php_uname('n');

			$ver_notice = UI::GetVersionNotice($version, '');
			echo $ver_notice;

			?>
				</div>

				<div class="lst-server-context">
					<a href="#" id="show-shortcut" class="lst-server-context-trigger" data-action="toggleShortcut" aria-haspopup="true" aria-expanded="false">
						<img class="lst-server-context-bolt" src="/res/img/server_bolt.svg" alt="">
						<span class="lst-server-context-name"><?php echo UIBase::Escape($host_name); ?></span>
						<i class="lst-icon" data-lucide="chevron-down"></i>
					</a>

					<div id="shortcut" tabindex="-1" aria-hidden="true" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_quickactions')); ?>">
						<ul class="lst-shortcut-grid">
							<li>
								<a href="#" class="lst-shortcut-tile lst-shortcut-tile--restart" data-lst-call="lst_restart"> <span class="iconbox"> <i class="lst-icon lst-icon--tile" data-lucide="refresh-ccw-dot"></i> <span><?php echo DMsg::UIStr('menu_restart')?> </span> </span> </a>
							</li>
							<li>
								<a href="index.php?view=realtimestats" class="lst-shortcut-tile lst-shortcut-tile--stats"> <span class="iconbox"> <i class="lst-icon lst-icon--tile" data-lucide="activity"></i> <span><?php echo DMsg::UIStr('menu_rtstats')?></span> </span> </a>
							</li>
							<li>
								<a href="index.php?view=logviewer" class="lst-shortcut-tile lst-shortcut-tile--logs"> <span class="iconbox"> <i class="lst-icon lst-icon--tile" data-lucide="list"></i> <span><?php echo DMsg::UIStr('menu_logviewer')?></span> </span> </a>
							</li>
							<li>
							<a href="#" class="lst-shortcut-tile lst-shortcut-tile--debug" data-lst-call="lst_toggledebug"> <span class="iconbox"> <i class="lst-icon lst-icon--tile" data-lucide="bug"></i> <span><?php echo DMsg::UIStr('service_toggledebuglabel')?></span> </span> </a>
							</li>
						</ul>
					</div>
				</div>
			</div>

			<div class="lst-header-actions">
				<div id="theme-switcher" class="transparent lst-header-control">
					<span><a href="#" rel="tooltip" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_toggletheme')); ?>" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_toggletheme')); ?>" data-action="toggleTheme" data-theme-light-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_lightmode')); ?>" data-theme-dark-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_darkmode')); ?>"><i class="lst-icon" data-lucide="moon"></i></a></span>
				</div>

				<ul class="header-dropdown-list lst-hide-phone">
					<li><?php echo UIBase::Get_LangDropdown(); ?></li>
				</ul>

				<div id="logout" class="transparent lst-header-control">
					<span><a href="/login.php?logoff=1" rel="tooltip" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_signout')); ?>" data-action="userLogout" data-logout-msg="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_logout')); ?>" data-logout-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_signoutof')); ?>" data-logout-cancel-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_cancel')); ?>" data-logout-confirm-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('note_signout')); ?>"><i class="lst-icon" data-lucide="log-out"></i></a></span>
				</div>
			</div>

		</header>
