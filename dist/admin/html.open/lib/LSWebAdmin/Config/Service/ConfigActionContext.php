<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\Product\Current\DTblDef;
use LSWebAdmin\UI\ConfUiContext;

class ConfigActionContext
{
    private $_routeState;
    private $_mutationDisplay;
    private $_uiContext;
    private $_tableDefClass;

    public function __construct($routeState = null, $mutationDisplay = null, $uiContext = null, $tableDefClass = null)
    {
        $this->_routeState = $routeState;
        $this->_mutationDisplay = $mutationDisplay;
        $this->_uiContext = $uiContext;
        $this->_tableDefClass = ($tableDefClass != null) ? $tableDefClass : DTblDef::class;
    }

    public static function fromDisplay($display, $tableDefClass = null)
    {
        if ($display instanceof self) {
            return $display;
        }

        $routeState = null;
        $uiContext = null;
        if (is_object($display) && method_exists($display, 'GetRouteState')) {
            $routeState = $display->GetRouteState();
            $uiContext = ConfUiContext::fromDisplay($display);
        }

        return new self(
            $routeState,
            $display,
            $uiContext,
            $tableDefClass
        );
    }

    public function GetAct()
    {
        if ($this->_routeState != null && method_exists($this->_routeState, 'GetAct')) {
            return $this->_routeState->GetAct();
        }

        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'GetAct')) {
            return $this->_mutationDisplay->GetAct();
        }

        return null;
    }

    public function SetAct($act)
    {
        if ($this->_routeState != null && method_exists($this->_routeState, 'SetAct')) {
            $this->_routeState->SetAct($act);
            return;
        }

        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'SetAct')) {
            $this->_mutationDisplay->SetAct($act);
        }
    }

    public function SetViewName($viewName)
    {
        if ($this->_routeState != null && method_exists($this->_routeState, 'SetViewName')) {
            $this->_routeState->SetViewName($viewName);
            return;
        }

        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'SetViewName')) {
            $this->_mutationDisplay->SetViewName($viewName);
        }
    }

    public function AddTopMsg($message)
    {
        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'AddTopMsg')) {
            $this->_mutationDisplay->AddTopMsg($message);
        }
    }

    public function SwitchToSubTid($extracted)
    {
        if ($this->_routeState != null && method_exists($this->_routeState, 'SwitchToSubTid')) {
            $this->_routeState->SwitchToSubTid($extracted, $this->_tableDefClass);
            return;
        }

        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'SwitchToSubTid')) {
            $this->_mutationDisplay->SwitchToSubTid($extracted);
        }
    }

    public function TrimLastId()
    {
        if ($this->_routeState != null && method_exists($this->_routeState, 'TrimLastId')) {
            $this->_routeState->TrimLastId();
            return;
        }

        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'TrimLastId')) {
            $this->_mutationDisplay->TrimLastId();
        }
    }

    public function GetActionData($actions)
    {
        if (is_object($this->_uiContext) && method_exists($this->_uiContext, 'GetActionData')) {
            $actionData = $this->_uiContext->GetActionData($actions);
            if (!empty($actionData)) {
                return $actionData;
            }
        }

        if (is_object($this->_mutationDisplay) && method_exists($this->_mutationDisplay, 'GetActionData')) {
            return $this->_mutationDisplay->GetActionData($actions);
        }

        return [];
    }

    public function GetMutationDisplay()
    {
        return $this->_mutationDisplay;
    }

    public function GetTableDefClass()
    {
        return $this->_tableDefClass;
    }
}
