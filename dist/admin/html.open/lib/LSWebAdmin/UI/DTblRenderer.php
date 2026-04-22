<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\DAttr;
use LSWebAdmin\Product\Current\UI;

class DTblRenderer
{
    private $_table;
    public function __construct(DTbl $table)
    {
        $this->_table = $table;
    }

    public function PrintHtml($dlayer, $disp)
    {
        $uiContext = ConfUiContext::fromDisplay($disp);
        $holderIndex = $this->_table->getHolderIndex();

        if ($holderIndex != null && $dlayer != null) {
            if (is_array($dlayer)) {
                foreach ($dlayer as $key => $nd) {
                    if ($nd->GetChildren($holderIndex) == null) {
                        $nd->AddChild(new CNode($holderIndex, $nd->Get(CNode::FLD_VAL)));
                    }
                }
            } elseif ($dlayer->GetChildren($holderIndex) == null) {
                $dlayer->AddChild(new CNode($holderIndex, $dlayer->Get(CNode::FLD_VAL)));
            }
        }

        if ($uiContext->IsViewAction()) {
            echo $this->renderView($dlayer, $uiContext) . " \n";
        } else {
            echo $this->renderEdit($dlayer, $uiContext) . " \n";
        }
    }

    private function hasContextOrderColumn()
    {
        foreach ($this->_table->getAttrs() as $attr) {
            if ($attr->_type == 'ctxseq') {
                return true;
            }
        }

        return false;
    }

    private function supportsContextOrderEnhancement()
    {
        return ($this->_table->isTop() && $this->hasContextOrderColumn() && !defined('_CONF_READONLY_'));
    }

    private function hasVisibleTypeColumn()
    {
        foreach ($this->_table->getAttrs() as $attr) {
            if (!$attr->IsFlagOn(DAttr::BM_HIDE) && $attr->GetKey() == 'type') {
                return true;
            }
        }

        return false;
    }

    private function resolveInlineRowIconName($attr, $data)
    {
        $iconMap = $this->_table->getIcon();
        if (!is_array($iconMap)) {
            return null;
        }

        $targetKey = $this->hasVisibleTypeColumn() ? 'type' : $this->_table->getHolderIndex();
        if ($targetKey == null || $attr->GetKey() != $targetKey) {
            return null;
        }

        $type = (string) $data->GetChildVal('type');
        if ($type !== '' && isset($iconMap[$type])) {
            return $iconMap[$type];
        }

        if (isset($iconMap['null'])) {
            return $iconMap['null'];
        }

        return 'file-text';
    }

    private function getInlineRowIconHtml($iconName)
    {
        return '<span class="lst-table-inline-icon" aria-hidden="true">'
            . '<i class="lst-icon" data-lucide="' . UIBase::EscapeAttr($iconName) . '"></i>'
            . '</span>';
    }

    private function getContextOrderControlsHtml()
    {
        $editLabel = DMsg::UIStr('btn_edit') . ' ' . DMsg::ALbl('l_order');

        return '<div class="lst-widget-controls lst-context-order-controls" data-lst-context-order-controls>'
            . '<button type="button" class="lst-widget-control-btn lst-widget-control-btn--labeled" data-lst-context-order-open>'
            . '<span class="lst-widget-control-btn__label"><i class="lst-icon" data-lucide="arrow-up-down"></i></span>'
            . '<span class="lst-widget-control-btn__text">' . UIBase::Escape($editLabel) . '</span>'
            . '</button>'
            . '</div>';
    }

    private function getClientFilterControlsHtml()
    {
        $filterId = $this->_table->getId();
        $filterLabel = DMsg::UIStr('note_filter');

        return '<div class="lst-widget-toolbar lst-widget-toolbar--table-filter">'
            . '<label class="lst-microfilter lst-microfilter--header" for="lst-filter-' . UIBase::EscapeAttr($filterId) . '">'
            . '<span class="lst-search-addon" aria-hidden="true"><i class="lst-icon" data-lucide="search"></i></span>'
            . '<input id="lst-filter-' . UIBase::EscapeAttr($filterId) . '" class="lst-choice-control" type="text"'
            . ' placeholder="' . UIBase::EscapeAttr($filterLabel) . '"'
            . ' aria-label="' . UIBase::EscapeAttr($filterLabel) . '"'
            . ' data-lst-table-filter="' . UIBase::EscapeAttr($filterId) . '">'
            . '</label>'
            . '<span class="lst-widget-toolbar__meta" data-lst-table-filter-meta="' . UIBase::EscapeAttr($filterId) . '"></span>'
            . '</div>';
    }

    private function shouldRenderClientFilter($dlayer = null)
    {
        if (!$this->_table->isTop()) {
            return false;
        }

        // Table controls subsume client filter when active
        if ($this->_table->hasTableControls()) {
            return false;
        }

        if (!$this->_table->hasClientFilter()) {
            return false;
        }

        if ($dlayer === null) {
            return true;
        }

        if (!is_array($dlayer)) {
            return false;
        }

        return count($dlayer) > $this->_table->getClientFilterThreshold();
    }

    private function shouldRenderTableControls($dlayer = null)
    {
        if (!$this->_table->isTop() || !$this->_table->hasTableControls()) {
            return false;
        }

        if ($dlayer === null) {
            return true;
        }

        if (!is_array($dlayer)) {
            return false;
        }

        return count($dlayer) > $this->_table->getTableControlsThreshold();
    }

    private function getTableControlsHeaderHtml()
    {
        $tableId = $this->_table->getId();
        $filterLabel = DMsg::UIStr('note_filter');

        return '<div class="lst-widget-toolbar lst-widget-toolbar--table-controls" data-lst-table-controls-header="' . UIBase::EscapeAttr($tableId) . '">'
            . '<label class="lst-microfilter lst-microfilter--header" for="lst-tctrl-filter-' . UIBase::EscapeAttr($tableId) . '">'
            . '<span class="lst-search-addon" aria-hidden="true"><i class="lst-icon" data-lucide="search"></i></span>'
            . '<input id="lst-tctrl-filter-' . UIBase::EscapeAttr($tableId) . '" class="lst-choice-control" type="text"'
            . ' placeholder="' . UIBase::EscapeAttr($filterLabel) . '"'
            . ' aria-label="' . UIBase::EscapeAttr($filterLabel) . '"'
            . ' data-lst-tctrl-filter="' . UIBase::EscapeAttr($tableId) . '">'
            . '</label>'
            . '<span class="lst-widget-toolbar__meta" data-lst-tctrl-meta="' . UIBase::EscapeAttr($tableId) . '"></span>'
            . '</div>';
    }

    private function getTableControlsFooterHtml($totalRows)
    {
        $tableId = $this->_table->getId();
        $rowsLabel = DMsg::UIStr('note_rows');

        $buf = '<div class="lst-table-controls-footer" data-lst-tctrl-footer="' . UIBase::EscapeAttr($tableId) . '"'
            . ' data-lst-tctrl-total="' . (int) $totalRows . '">';

        // Rows-per-page selector
        $buf .= '<div class="lst-table-controls-footer__rows">'
            . '<label for="lst-tctrl-pagesize-' . UIBase::EscapeAttr($tableId) . '">'
            . '<select id="lst-tctrl-pagesize-' . UIBase::EscapeAttr($tableId) . '"'
            . ' class="lst-choice-control lst-control-compact"'
            . ' data-lst-tctrl-pagesize="' . UIBase::EscapeAttr($tableId) . '">'
            . '<option value="10">10</option>'
            . '<option value="25">25</option>'
            . '<option value="50">50</option>'
            . '<option value="100">100</option>'
            . '<option value="0">' . UIBase::Escape(DMsg::UIStr('service_all')) . '</option>'
            . '</select>'
            . '<span class="lst-table-controls-footer__rows-label">' . UIBase::Escape($rowsLabel) . '</span>'
            . '</label></div>';

        // Info
        $buf .= '<span class="lst-table-controls-footer__info" data-lst-tctrl-info="' . UIBase::EscapeAttr($tableId) . '"></span>';

        // Pagination
        $buf .= '<nav class="lst-table-controls-footer__paging" data-lst-tctrl-paging="' . UIBase::EscapeAttr($tableId) . '" aria-label="Table pagination">'
            . '<button type="button" class="lst-tctrl-page-btn" data-lst-tctrl-prev="' . UIBase::EscapeAttr($tableId) . '" disabled>'
            . '<i class="lst-icon" data-lucide="chevron-left"></i></button>'
            . '<span class="lst-tctrl-page-numbers" data-lst-tctrl-pages="' . UIBase::EscapeAttr($tableId) . '"></span>'
            . '<button type="button" class="lst-tctrl-page-btn" data-lst-tctrl-next="' . UIBase::EscapeAttr($tableId) . '" disabled>'
            . '<i class="lst-icon" data-lucide="chevron-right"></i></button>'
            . '</nav>';

        $buf .= '</div>';
        return $buf;
    }

    private function getPrintHeader($disp, $actString, $isEdit = false, $hasSort = false)
    {
        $buf = '<header role="heading" class="lst-widget-header">';
        $tableHelp = ' ';
        $helpKey = $this->_table->getHelpKey();
        $attrs = $this->_table->getAttrs();

        if ($helpKey != null && ($dhelp_item = DMsg::GetAttrTip($helpKey)) != null) {
            $tableHelp = $dhelp_item->Render();
        } elseif (count($attrs) == 1 && $this->_table->getCols() == 1) {
            $av = array_values($attrs);
            $a0 = $av[0];
            if ($a0->_label == null || $a0->_label == $this->_table->getTitle()) {
                if (($dhelp_item = DMsg::GetAttrTip($a0->_helpKey)) != null) {
                    $is_blocked = $a0->blockedVersion();
                    $version = $is_blocked ? $a0->_version : 0;
                    $reason = $is_blocked ? $a0->blockedReason() : '';
                    $tableHelp = $dhelp_item->Render($version, $reason);
                }
            }
        }

        $title = $this->_table->getTitle();
        if ($isEdit) {
            $title = '<i class="lst-icon" data-lucide="edit"></i> ' . $title;
        }

        $ref = $disp->GetRef();
        if ($this->_table->shouldShowParentRef() && $ref != null) {
            $pos = strpos($ref, '`');
            if ($pos !== false) {
                $title .= ' - ' . substr($ref, 0, $pos);
            } else {
                $title .= ' - ' . $ref;
            }
        }

        $all_blocked = true;
        $keys = array_keys($attrs);
        foreach ($keys as $i) {
            if (!$attrs[$i]->blockedVersion()) {
                $all_blocked = false;
                break;
            }
        }
        if ($actString && ($all_blocked || defined('_CONF_READONLY_'))) {
            $actString = (strpos($actString, 'B') !== false) ? 'B' : null;
        }

        if (!$isEdit && $this->shouldRenderTableControls()) {
            $buf .= $this->getTableControlsHeaderHtml();
        } elseif (!$isEdit && $this->shouldRenderClientFilter()) {
            $buf .= $this->getClientFilterControlsHtml();
        }
        if ($actString != null) {
            $actdata = $disp->GetActionData($actString, $this->_table->getId(), '', $this->_table->getAddTbl());
            $buf .= UI::GetActionButtons($actdata, 'toolbar');
        }
        if (!$isEdit && $this->supportsContextOrderEnhancement()) {
            $buf .= $this->getContextOrderControlsHtml();
        }
        $buf .= '<div class="lst-widget-heading"><h2>' . $title . '</h2>';
        if (trim($tableHelp) != '') {
            $buf .= '<span class="lst-help-slot">' . $tableHelp . '</span>';
        }
        $buf .= '</div></header>';

        if ($this->_table->isTop()) {
            $buf .= '<thead><tr>';
            $url = $disp->GetCtrlUrl();
            if ($disp->GetTid()) {
                $url .= '&t=' . urlencode($disp->GetTid());
            }
            if ($disp->GetRef()) {
                $url .= '&r=' . urlencode($disp->GetRef());
            }

            $align = $this->_table->getAlign();
            foreach ($keys as $i) {
                $attr = $attrs[$i];
                if ($attr->IsFlagOn(DAttr::BM_HIDE)) {
                    continue;
                }

                $buf .= '<th';
                $thClasses = [];
                if (isset($align[$i]) && $align[$i] != 'left') {
                    $thClasses[] = 'text-' . $align[$i];
                }
                if ($attr->_type == 'action') {
                    $thClasses[] = 'lst-action-col';
                    if (!in_array('lst-text-right', $thClasses, true)) {
                        $thClasses[] = 'lst-text-right';
                    }
                }

                $sortState = null;
                if ($hasSort && $attr->_type != 'action') {
                    $sortState = 'none';
                }

                if (!empty($thClasses)) {
                    $buf .= ' class="' . implode(' ', $thClasses) . '"';
                }
                if ($sortState != null) {
                    $buf .= ' data-lst-sort-state="' . $sortState . '" aria-sort="none" tabindex="0"';
                }

                $buf .= '>';
                $buf .= $attr->_label;
                if ($attr->_type == 'ctxseq') {
                    $attr->_hrefLink = $url . $attr->_href;
                }
                $buf .= '</th>';
            }
            $buf .= "</tr></thead>\n";
        }

        return $buf;
    }

    private function renderView($dlayer, $disp)
    {
        $supportsContextOrder = $this->supportsContextOrderEnhancement();
        $hasClientFilter = $this->shouldRenderClientFilter($dlayer);
        $hasTableControls = $this->shouldRenderTableControls($dlayer);
        $totalRows = (is_array($dlayer)) ? count($dlayer) : 0;
        $buf = '<div class="lst-widget lst-config-view-widget';
        if ($supportsContextOrder) {
            $buf .= ' lst-context-order-widget';
        }
        $buf .= '"';
        if ($supportsContextOrder) {
            $buf .= ' data-lst-context-order-widget="1"'
                . ' data-lst-context-order-title="' . UIBase::EscapeAttr(DMsg::UIStr('btn_edit') . ' ' . DMsg::ALbl('l_order')) . '"'
                . ' data-lst-context-order-save-label="' . UIBase::EscapeAttr(DMsg::UIStr('btn_save') . ' ' . DMsg::ALbl('l_order')) . '"'
                . ' data-lst-context-order-cancel-label="' . UIBase::EscapeAttr(DMsg::UIStr('btn_cancel')) . '"';
        }
        $buf .= '>';

        if ($supportsContextOrder) {
            $buf .= '<input type="hidden" value="" data-lst-context-order-payload data-lst-context-order-field="ctxorder">';
        }

        $tableClass = 'table lst-table-bordered lst-sortable-table';
        if ($this->_table->isTop()) {
            $tableClass .= ' lst-config-list-table';
        }

        $buf .= '<table class="' . $tableClass . '"';
        if ($hasClientFilter) {
            $buf .= ' data-lst-filter-table="' . UIBase::EscapeAttr($this->_table->getId()) . '"';
        }
        if ($hasTableControls) {
            $buf .= ' data-lst-tctrl-table="' . UIBase::EscapeAttr($this->_table->getId()) . '"';
        }
        $buf .= '>' . "\n";
        $ref = $disp->GetLastRef();
        $disptid = $disp->GetTid();
        $hasB = ($disptid != '');
        $attrs = $this->_table->getAttrs();

        if ($this->_table->isTop()) {
            if ($this->_table->getAddTbl() == null) {
                $actString = 'E';
            } elseif ($this->_table->getAddTbl() != 'N') {
                $actString = 'a';
            } else {
                $actString = '';
            }

            if ($hasB) {
                $actString .= 'B';
            }

            $hasSort = ($dlayer != null && is_array($dlayer));
            $buf .= $this->getPrintHeader($disp, $actString, false, $hasSort && !$hasClientFilter && !$hasTableControls);
            $buf .= '<tbody>';

            if ($dlayer != null) {
                if (!is_array($dlayer)) {
                    $dlayer = [$dlayer->Get(CNode::FLD_VAL) => $dlayer];
                }

                $keys = array_keys($dlayer);

                $action_attr = null;
                foreach ($attrs as $attr) {
                    if ($attr->_type == 'action') {
                        if ($attr->blockedVersion()) {
                            $attr->_maxVal = '';
                        } elseif ($attr->IsFlagOn(DAttr::BM_NOTNULL) && strpos($attr->_maxVal, 'd') !== false && count($dlayer) == 1) {
                            $attr->_maxVal = str_replace('d', '', $attr->_maxVal);
                        }
                        $action_attr = $attr;
                        break;
                    }
                }

                $index = 0;
                foreach ($keys as $key) {
                    $nd = $dlayer[$key];
                    $buf .= $this->getPrintLineMulti($nd, $key, $index, $disp, $action_attr);
                    $index++;
                }
            }
        } else {
            $actString = 'E';
            if ($hasB) {
                $actString .= 'B';
            }
            if ($ref != null && is_array($dlayer)) {
                $dlayer = $dlayer[$ref];
            }

            $buf .= $this->getPrintHeader($disp, $actString);
            $buf .= '<tbody>';

            foreach ($attrs as $attr) {
                $buf .= $this->getPrintLine($dlayer, $disp, $attr);
            }
        }

        $buf .= '</tbody></table>';
        if ($hasTableControls) {
            $buf .= $this->getTableControlsFooterHtml($totalRows);
        }
        $buf .= '</div>';
        return $buf;
    }

    private function renderEdit($dlayer, $disp)
    {
        $buf = '';
        $ref = $disp->GetLastRef();

        if ($ref != null && is_array($dlayer)) {
            $dlayer = $dlayer[$ref];
        }

        $labels = [$this->_table->getHelpKey()];
        foreach ($this->_table->getAttrs() as $attr) {
            $labels[] = $attr->_helpKey;
        }
        if (($tips = DMsg::GetEditTips($labels)) != null) {
            $buf .= UI::GetTblTips($tips);
        }

        $buf .= '<div class="lst-widget lst-config-edit-widget">' . "\n";

        $actString = ((substr($this->_table->getId(), -3) == 'SEL') ? 'n' : 's') . 'B';
        $buf .= $this->getPrintHeader($disp, $actString, true);

        $buf .= '<div role="content" class="lst-widget-content lst-config-edit-content"><div class="lst-widget-body lst-form lst-config-edit-form"><div class="lst-config-edit-fieldset">';
        foreach ($this->_table->getAttrs() as $attr) {
            $buf .= $this->getPrintInputline($dlayer, $disp, $attr);
        }

        $buf .= '</div></div></div></div>';
        return $buf;
    }

    private function getPrintLine($node, $disp, $attr)
    {
        $valwid = 0;
        if ($attr == null || $attr->IsFlagOn(DAttr::BM_HIDE)) {
            return '';
        }

        $is_blocked = $attr->blockedVersion();
        $version = $is_blocked ? $attr->_version : 0;
        $reason = $is_blocked ? $attr->blockedReason() : '';
        if ($attr->_type == 'sel1' && $node != null && $node->GetChildVal($attr->GetKey()) != null) {
            $attr->SetDerivedSelOptions($disp->GetDerivedSelOptions($this->_table->getId(), $attr->_minVal, $node));
        }
        $buf = '<tr>';
        if ($attr->_label) {
            if ($is_blocked) {
                $buf .= '<td class="xtbl_label_blocked">';
            } else {
                $buf .= '<td class="xtbl_label">';
            }
            $buf .= $attr->_label;

            $dhelp_item = DMsg::GetAttrTip($attr->_helpKey);
            if ($this->_table->getCols() == 1) {
                $buf .= '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
            } else {
                if ($dhelp_item != null) {
                    $buf .= '<span class="lst-inline-help">' . $dhelp_item->Render($version, $reason) . '</span>';
                }

                $buf .= '</td>';
            }

            $buf .= '</td>';
        }

        if ($this->_table->getCols() == 1 && $attr->_label) {
            $buf .= '</tr><tr>';
        }

        $buf .= '<td';

        if ($attr->blockedVersion()) {
            $buf .= ' class="xtbl_value_blocked"';
        }
        if ($valwid > 0) {
            $buf .= ' width="' . $valwid . '"';
        }
        $buf .= '>';

        if ($attr->_href) {
            $link = $disp->GetCtrlUrl();
            $t = $disp->GetTid();
            $r = $disp->GetRef();
            if ($t) {
                $link .= '&t=' . $t;
            }
            if ($r) {
                $r = urlencode($r);
                $link .= '&r=' . $r;
            }

            $link .= $attr->_href;
            $attr->_hrefLink = $r ? str_replace('$R', $r, $link) : $link;
        }

        $buf .= $attr->toHtml($node);
        $buf .= "</td></tr>\n";
        return $buf;
    }

    private function getPrintInputline($dlayer, $disp, $attr)
    {
        if ($attr->IsFlagOn(DAttr::BM_NOEDIT)) {
            return '';
        }

        if ($attr->_type == 'sel1') {
            $attr->SetDerivedSelOptions($disp->GetDerivedSelOptions($this->_table->getId(), $attr->_minVal, $dlayer));
        }

        $is_blocked = $attr->blockedVersion();
        $helppop = '';

        if (($dhelp_item = DMsg::GetAttrTip($attr->_helpKey)) != null) {
            $helppop = '<span class="lst-help-slot">' . $dhelp_item->Render(
                $is_blocked ? $attr->_version : 0,
                $is_blocked ? $attr->blockedReason() : ''
            ) . '</span>';
        }

        return $attr->toInputGroup($dlayer, $is_blocked, $helppop);
    }

    private function getPrintLineMulti($data, $key0, $htmlid, $disp, $action_attr)
    {
        $buf = '<tr';
        if ($this->supportsContextOrderEnhancement() && $this->_table->getHolderIndex() != null) {
            $orderId = $data->GetChildVal($this->_table->getHolderIndex());
            $orderPosition = $data->GetChildVal('order');
            if ($orderId != null && $orderId !== '') {
                $buf .= ' data-lst-context-order-row="1"'
                    . ' data-lst-order-id="' . UIBase::EscapeAttr(rawurlencode((string) $orderId)) . '"'
                    . ' data-lst-order-label="' . UIBase::EscapeAttr((string) $orderId) . '"';
                if ($orderPosition != null && $orderPosition !== '') {
                    $buf .= ' data-lst-order-position="' . UIBase::EscapeAttr((string) $orderPosition) . '"';
                }
            }
        }
        $buf .= '>';

        $attrs = $this->_table->getAttrs();
        $keys = array_keys($attrs);
        $actionLink = null;
        $indexActionLink = null;

        if ($action_attr != null) {
            if (is_array($action_attr->_minVal)) {
                $index = $action_attr->_minVal[0];
                $type = $data->GetChildVal($index);
                $ti = isset($action_attr->_minVal[$type]) ? $action_attr->_minVal[$type] : $action_attr->_minVal[1];
            } else {
                $ti = $action_attr->_minVal;
            }
            $actString = $action_attr->_maxVal;
            if ($actString && defined('_CONF_READONLY_')) {
                $actString = (strpos($actString, 'X') !== false) ? 'X' : 'v';
            }

            $actdata = $disp->GetActionData($actString, $ti, $key0);
            $actionLink = UI::GetActionButtons($actdata, 'icon');
            $indexActionLink = isset($actdata['v']) ? $actdata['v']['href'] : null;
        }

        $align = $this->_table->getAlign();
        foreach ($keys as $key) {
            $attr = $attrs[$key];
            if ($attr->IsFlagOn(DAttr::BM_HIDE)) {
                continue;
            }

            $buf .= '<td';
            $tdClasses = [];
            if ($attr->_type == 'action') {
                $tdClasses[] = 'lst-action-col';
            }
            if (!empty($tdClasses)) {
                $buf .= ' class="' . implode(' ', $tdClasses) . '"';
            }
            if (isset($align[$key])) {
                $buf .= ' align="' . $align[$key] . '"';
            }
            $buf .= '>';

            if ($attr->_type == 'action') {
                $buf .= $actionLink;
            } else {
                if ($attr->_type == 'sel1' && $data->GetChildVal($attr->GetKey()) != null) {
                    $attr->SetDerivedSelOptions($disp->GetDerivedSelOptions($this->_table->getId(), $attr->_minVal, $data));
                }

                if (($inlineIconName = $this->resolveInlineRowIconName($attr, $data)) != null) {
                    $buf .= $this->getInlineRowIconHtml($inlineIconName);
                }

                if ($attr->GetKey() == $this->_table->getHolderIndex()) {
                    $buf .= $attr->toHtml($data, $indexActionLink);

                    if ($this->_table->hasNote() && (($note = $data->GetChildVal('note')) != null)) {
                        $buf .= '<button type="button" class="lst-row-note" rel="tooltip" data-placement="right"
         data-original-title="' . htmlspecialchars(htmlspecialchars($note, ENT_QUOTES), ENT_QUOTES) . '" data-html="true">
        <i class="lst-icon" data-lucide="info"></i></button>';
                    }
                } else {
                    $buf .= $attr->toHtml($data, null);
                }
            }
            $buf .= '</td>';
        }
        $buf .= "</tr>\n";

        return $buf;
    }
}
