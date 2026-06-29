<?php

namespace LSWebAdmin\Config\Validation;

class ConfigDeleteValidationResult
{
    private $_messages;
    private $_warnings;

    public function __construct($messages = [], $warnings = [])
    {
        $this->_messages = [];
        foreach ((array) $messages as $message) {
            $this->AddMessage($message);
        }

        $this->_warnings = [];
        foreach ((array) $warnings as $warning) {
            $this->AddWarning($warning);
        }
    }

    public static function allow($warnings = [])
    {
        return new self([], $warnings);
    }

    public static function block($messages)
    {
        return new self($messages);
    }

    public function AddMessage($message)
    {
        if ($message !== null && $message !== '') {
            $this->_messages[] = (string) $message;
        }
    }

    public function AddWarning($warning)
    {
        if ($warning !== null && $warning !== '') {
            $this->_warnings[] = (string) $warning;
        }
    }

    public function IsAllowed()
    {
        return count($this->_messages) == 0;
    }

    public function GetMessages()
    {
        return $this->_messages;
    }

    public function HasWarnings()
    {
        return count($this->_warnings) > 0;
    }

    public function GetWarnings()
    {
        return $this->_warnings;
    }
}
