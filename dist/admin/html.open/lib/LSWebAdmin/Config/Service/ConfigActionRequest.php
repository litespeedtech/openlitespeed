<?php

namespace LSWebAdmin\Config\Service;

class ConfigActionRequest
{
    private $_context;
    private $_confData;
    private $_currentData;
    private $_validatePostHandler;
    private $_instantiateTemplateHandler;
    private $_uiClass;
    private $_ctxseq;
    private $_ctxOrderPayload;
    private $_ctxOrder;

    public function __construct(
        $context,
        $confData,
        $currentData = null,
        $validatePostHandler = null,
        $instantiateTemplateHandler = null,
        $uiClass = null,
        $ctxseq = 0,
        $ctxOrderPayload = ''
    ) {
        $this->_context = ConfigActionContext::fromDisplay($context);
        $this->_confData = $confData;
        $this->_currentData = $currentData;
        $this->_validatePostHandler = $validatePostHandler;
        $this->_instantiateTemplateHandler = $instantiateTemplateHandler;
        $this->_uiClass = $uiClass;
        $this->_ctxseq = (int) $ctxseq;
        $this->_ctxOrderPayload = is_string($ctxOrderPayload) ? trim($ctxOrderPayload) : '';
        $this->_ctxOrder = null;
    }

    public function GetContext()
    {
        return $this->_context;
    }

    public function GetMutationDisplay()
    {
        return $this->_context->GetMutationDisplay();
    }

    public function GetConfData()
    {
        return $this->_confData;
    }

    public function GetCurrentData()
    {
        return $this->_currentData;
    }

    public function GetAct()
    {
        return $this->_context->GetAct();
    }

    public function SetAct($act)
    {
        $this->_context->SetAct($act);
    }

    public function SetViewName($viewName)
    {
        $this->_context->SetViewName($viewName);
    }

    public function AddTopMsg($message)
    {
        $this->_context->AddTopMsg($message);
    }

    public function SwitchToSubTid($extracted)
    {
        $this->_context->SwitchToSubTid($extracted);
    }

    public function TrimLastId()
    {
        $this->_context->TrimLastId();
    }

    public function GetActionData($actions)
    {
        return $this->_context->GetActionData($actions);
    }

    public function GetCtxSeq()
    {
        return $this->_ctxseq;
    }

    public function GetCtxOrder()
    {
        if ($this->_ctxOrder !== null) {
            return $this->_ctxOrder;
        }

        $this->_ctxOrder = [];
        if ($this->_ctxOrderPayload === '') {
            return $this->_ctxOrder;
        }

        $segments = preg_split("/\r\n|\r|\n/", $this->_ctxOrderPayload);
        foreach ($segments as $segment) {
            $segment = trim($segment);
            if ($segment === '') {
                continue;
            }

            $this->_ctxOrder[] = rawurldecode($segment);
        }

        return $this->_ctxOrder;
    }

    public function ShouldApplyContextOrder()
    {
        return ($this->GetAct() === 'o' && count($this->GetCtxOrder()) > 0);
    }

    public function GetUiClass()
    {
        return $this->_uiClass;
    }

    public function GetTableDefClass()
    {
        return $this->_context->GetTableDefClass();
    }

    public function ValidatePost()
    {
        if (!is_callable($this->_validatePostHandler)) {
            return null;
        }

        return call_user_func($this->_validatePostHandler);
    }

    public function InstantiateTemplate()
    {
        if (!is_callable($this->_instantiateTemplateHandler)) {
            return false;
        }

        return (bool) call_user_func($this->_instantiateTemplateHandler);
    }
}
