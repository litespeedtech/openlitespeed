<?php

namespace LSWebAdmin\Config\Validation;

class ConfigValidationResult
{
    const STATUS_ERROR = -1;
    const STATUS_STOP = 0;
    const STATUS_OK = 1;

    private $_extracted;
    private $_viewName;
    private $_hasViewNameUpdate;
    private $_status;
    private $_messages;

    public function __construct($extracted, $viewName = null, $hasViewNameUpdate = false, $status = self::STATUS_OK, $messages = [])
    {
        $this->_extracted = $extracted;
        $this->_viewName = $viewName;
        $this->_hasViewNameUpdate = $hasViewNameUpdate;
        $this->_status = (int) $status;
        $this->_messages = is_array($messages) ? $messages : [];
    }

    public function GetExtracted()
    {
        return $this->_extracted;
    }

    public function HasErr()
    {
        return ($this->_status < self::STATUS_STOP || ($this->_extracted != null && $this->_extracted->HasErr()));
    }

    public function ShouldStopSave()
    {
        return ($this->_status == self::STATUS_STOP);
    }

    public function GetMessages()
    {
        return $this->_messages;
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
