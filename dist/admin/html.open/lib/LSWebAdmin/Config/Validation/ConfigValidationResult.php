<?php

namespace LSWebAdmin\Config\Validation;

class ConfigValidationResult
{
    private $_extracted;
    private $_viewName;
    private $_hasViewNameUpdate;

    public function __construct($extracted, $viewName = null, $hasViewNameUpdate = false)
    {
        $this->_extracted = $extracted;
        $this->_viewName = $viewName;
        $this->_hasViewNameUpdate = $hasViewNameUpdate;
    }

    public function GetExtracted()
    {
        return $this->_extracted;
    }

    public function HasErr()
    {
        return ($this->_extracted != null && $this->_extracted->HasErr());
    }

    public function GetViewName()
    {
        return $this->_viewName;
    }

    public function HasViewNameUpdate()
    {
        return $this->_hasViewNameUpdate;
    }
}
