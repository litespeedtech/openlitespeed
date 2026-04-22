<?php

use LSWebAdmin\UI\UIBase;

function lstNavLinkAttributes(array $item)
{
    $url = isset($item['url']) ? $item['url'] : '#';
    $title = isset($item['title']) ? $item['title'] : '(No Name)';

    $attrs = 'href="' . UIBase::EscapeAttr($url) . '" data-nav-title="' . UIBase::EscapeAttr($title) . '"';
    if (isset($item['url_target'])) {
        $attrs .= ' target="' . UIBase::EscapeAttr($item['url_target']) . '"';
        if ($item['url_target'] === '_blank') {
            $attrs .= ' rel="noopener noreferrer"';
        }
    }

    return $attrs;
}

function lstRenderNavItems(array $items, $isSubNav = false)
{
    $html = '<ul>';

    foreach ($items as $item) {
        $title = isset($item['title']) ? $item['title'] : '(No Name)';
        $iconBadge = isset($item['icon_badge']) ? '<em>' . UIBase::Escape($item['icon_badge']) . '</em>' : '';
        $icon = isset($item['icon']) ? '<i class="lst-icon" data-lucide="' . UIBase::EscapeAttr($item['icon']) . '">' . $iconBadge . '</i>' : '';
        $labelHtml = isset($item['label_htm']) ? $item['label_htm'] : '';
        $activeClass = isset($item['active']) ? ' class="active"' : '';

        $html .= '<li' . $activeClass . '>';
        $html .= '<a class="lst-nav-link" ' . lstNavLinkAttributes($item) . '>';
        $html .= $icon . ' <span class="menu-item-parent">' . UIBase::Escape($title) . '</span>' . $labelHtml . '</a>';

        if (!empty($item['sub']) && is_array($item['sub'])) {
            $html .= lstRenderNavItems($item['sub'], true);
        }

        $html .= '</li>';
    }

    $html .= '</ul>';
    return $html;
}

?>
		<aside id="left-panel">
			<nav>
				<?php echo lstRenderNavItems($page_nav); ?>
			</nav>
			<?php echo UIBase::Get_LangSidebarDropdown(); ?>
			<span class="lst-nav-minify" data-action="minifyMenu" tabindex="0" role="button" aria-label="Minify menu"> <i class="lst-icon" data-lucide="arrow-left-circle"></i> </span>

		</aside>
