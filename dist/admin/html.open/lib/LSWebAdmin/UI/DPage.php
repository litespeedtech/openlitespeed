<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Product\Base\DTblMap;
use LSWebAdmin\Product\Base\DTblSelectorMap;
use LSWebAdmin\Product\Current\DTblDef;

class DPage
{

    private $_id;
    private $_label;
    private $_tblmap;
    private $_printdone;
    private $_disp_tid;
    private $_disp_ref;
    private $_extended;
    private $_linked_tbls;
    private $_wideLabels = false;

    public function __construct($id, $label, $tblmap)
    {
        $this->_id = $id;
        $this->_label = $label;
        $this->_tblmap = $tblmap;
    }

    public function GetID()
    {
        return $this->_id;
    }

    public function GetLabel()
    {
        return $this->_label;
    }

    public function GetTblMap()
    {
        return $this->_tblmap;
    }

    /**
     * Opt this page into the wider label column preset (~310px instead of 230px).
     * Use it on pages whose labels are long enough to wrap with the default width,
     * such as Server -> Tuning. Affects every table on the page so the label
     * column stays consistent across the page.
     */
    public function useWideLabels()
    {
        $this->_wideLabels = true;
        return $this;
    }

    public function hasWideLabels()
    {
        return $this->_wideLabels;
    }

    public function PrintHtml($disp)
    {
        $uiContext = ConfUiContext::fromDisplay($disp);
        $this->_disp_tid = $uiContext->GetTid();
        $this->_disp_ref = $uiContext->GetRef();

        $this->_linked_tbls = null;
        $this->_extended = true;
        if ($this->_disp_tid == '') {
            $this->_extended = false;
        } elseif ($this->_disp_tid && ($last = strrpos($this->_disp_tid, '`')) > 0) {
            $this->_disp_tid = substr($this->_disp_tid, $last + 1);
        }

        if (($topmesg = $uiContext->GetTopMsg()) != null) {

            foreach ($topmesg as $tm) {
                echo self::renderTopMessage($tm);
            }
        }

        $root = $uiContext->GetPageData();
        if ($root == null)
            return;


        if ($root->Get(CNode::FLD_KEY) == CNode::K_EXTRACTED) {
            $this->print_tbl($this->_disp_tid, $root, $uiContext);
        } else {
            $this->_printdone = false;
            $this->print_map($this->_tblmap, $root, $uiContext, $this->_disp_ref);
        }

        if ($uiContext->IsViewAction() && $this->_linked_tbls != null) {
            $this->_extended = true;
            $uiContext->SetPrintingLinked(true);
            foreach ($this->_linked_tbls as $lti) {
                $this->_disp_tid = $lti;
                $this->_disp_ref = $uiContext->GetRef();
                $this->_printdone = false;
                $this->print_map($this->_tblmap, $root, $uiContext, $this->_disp_ref);
            }
            $uiContext->SetPrintingLinked(false);
        }
    }

    private function print_map($tblmap, $node, $disp, $ref = '')
    {
        $dlayer = ($node == null) ? null : $node->LocateLayer($tblmap->GetLoc());
        $maps = $tblmap->GetMaps($this->_extended);
        foreach ($maps as $m) {
            if ($m instanceof DTblMap) {
                list($selectedLayer, $nextref) = $this->selectLayerByRef($dlayer, $ref);
                $this->print_map($m, $selectedLayer, $disp, $nextref);
                if ($this->_printdone)
                    break;
            } elseif ($m instanceof DTblSelectorMap) {
                list($selectedLayer, $nextref) = $this->selectLayerByRef($dlayer, $ref);
                $this->print_resolved_maps($m->Resolve($selectedLayer), $selectedLayer, $disp, $nextref);
                if ($this->_printdone)
                    break;
            } else {
                if ($m != null && ($this->_disp_tid == '' || $this->_disp_tid == $m)) {
                    $this->print_tbl($m, $dlayer, $disp);
                    if ($this->_disp_tid == $m) {
                        $this->_printdone = true;
                        break;
                    }
                }
            }
        }
    }

    private function print_resolved_maps($maps, $dlayer, $disp, $ref)
    {
        foreach ($maps as $m) {
            if ($m instanceof DTblMap) {
                $this->print_map($m, $dlayer, $disp, $ref);
                if ($this->_printdone)
                    break;
            } elseif ($m != null && ($this->_disp_tid == '' || $this->_disp_tid == $m)) {
                $this->print_tbl($m, $dlayer, $disp);
                if ($this->_disp_tid == $m) {
                    $this->_printdone = true;
                    break;
                }
            }
        }
    }

    private function selectLayerByRef($dlayer, $ref)
    {
        if (!is_array($dlayer)) {
            return [$dlayer, $ref];
        }

        if (($first = strpos($ref, '`')) > 0) {
            $nextref = substr($ref, $first + 1);
            $ref = substr($ref, 0, $first);
        } else {
            $nextref = '';
        }

        return [isset($dlayer[$ref]) ? $dlayer[$ref] : null, $nextref];
    }

    private function print_tbl($tid, $dlayer, $disp)
    {
        $tbl = DTblDef::getInstance()->GetTblDef($tid);
        $tbl->PrintHtml($dlayer, $disp);

        if (($linked = $tbl->Get(DTbl::FLD_LINKEDTBL)) != null) {
            if ($this->_linked_tbls == null)
                $this->_linked_tbls = $linked;
            else
                $this->_linked_tbls = array_merge($this->_linked_tbls, $linked);
        }
    }

    private static function renderTopMessage($message)
    {
        if (!is_array($message)) {
            return UIBase::message('', $message, 'error');
        }

        $messageType = isset($message['type']) ? strtolower((string) $message['type']) : 'danger';
        if ($messageType === 'error' || $messageType === 'normal' || $messageType === '') {
            $messageType = 'danger';
        }
        if (!in_array($messageType, ['success', 'info', 'warning', 'danger'], true)) {
            $messageType = 'danger';
        }

        $titleHtml = '';
        if (isset($message['title']) && $message['title'] !== '') {
            $titleHtml = '<strong>' . UIBase::Escape($message['title']) . '</strong> ';
        }

        $text = isset($message['text']) ? $message['text'] : '';
        return '<div class="lst-message lst-message--' . UIBase::EscapeAttr($messageType) . '">'
            . $titleHtml . $text . '</div>';
    }

}
