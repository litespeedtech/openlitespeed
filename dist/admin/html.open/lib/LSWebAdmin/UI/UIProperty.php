<?php

namespace LSWebAdmin\UI;

class UIProperty
{
    const FLD_PAGE_TITLE = 1;

    const FLD_FORM_ACTION = 2;
    const FLD_FORM_HIDDENVARS = 3;
    const FLD_TABS = 4;
    const FLD_SERVER_NAME = 5;

    private $servername;
    private $page_title;

    public $no_main_header = false;

    private $form_action;
    private $form_hiddenvars;
    private $tabs;

    public function Get($field)
    {
        switch ($field) {
            case self::FLD_PAGE_TITLE:
                return $this->page_title;
            case self::FLD_FORM_ACTION:
                return $this->form_action;
            case self::FLD_FORM_HIDDENVARS:
                return $this->form_hiddenvars;
            case self::FLD_TABS:
                return $this->tabs;
            case self::FLD_SERVER_NAME:
                return $this->servername;
            default:
                trigger_error("UIProperty::Get illegal field $field", E_USER_ERROR);
        }
    }

    public function Set($field, $val)
    {
        switch ($field) {
            case self::FLD_PAGE_TITLE:
                $this->page_title = $val;
                break;
            case self::FLD_FORM_ACTION:
                $this->form_action = $val;
                break;
            case self::FLD_FORM_HIDDENVARS:
                $this->form_hiddenvars = $val;
                break;
            case self::FLD_TABS:
                $this->tabs = $val;
                break;
            case self::FLD_SERVER_NAME:
                $this->servername = $val;
                break;
            default:
                trigger_error("UIProperty::Set illegal field $field", E_USER_ERROR);
        }
    }

}
