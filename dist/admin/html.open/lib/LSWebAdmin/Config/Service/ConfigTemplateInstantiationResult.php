<?php

namespace LSWebAdmin\Config\Service;

class ConfigTemplateInstantiationResult
{
    private $_success;
    private $_messages;

    public function __construct($success, $messages = null)
    {
        $this->_success = (bool) $success;
        $this->_messages = is_array($messages) ? $messages : [];
    }

    public function IsSuccessful()
    {
        return $this->_success;
    }

    public function GetMessages()
    {
        return $this->_messages;
    }
}
