<?php

namespace LSWebAdmin\Config\Service;

class ConfigBootstrapResult
{
    private $_serverData;
    private $_adminData;
    private $_currentData;
    private $_specialData;
    private $_messages = [];

    public function SetServerData($data)
    {
        $this->_serverData = $data;
    }

    public function GetServerData()
    {
        return $this->_serverData;
    }

    public function SetAdminData($data)
    {
        $this->_adminData = $data;
    }

    public function GetAdminData()
    {
        return $this->_adminData;
    }

    public function SetCurrentData($data)
    {
        $this->_currentData = $data;
    }

    public function GetCurrentData()
    {
        return $this->_currentData;
    }

    public function SetSpecialData($data)
    {
        $this->_specialData = $data;
    }

    public function GetSpecialData()
    {
        return $this->_specialData;
    }

    public function AddMessage($message)
    {
        if (is_string($message) && $message !== '') {
            $this->_messages[] = $message;
        }
    }

    public function GetMessages()
    {
        return $this->_messages;
    }

    public function GetLoadedData()
    {
        $loaded = [];
        if ($this->_serverData != null) {
            $loaded[] = $this->_serverData;
        }
        if ($this->_adminData != null) {
            $loaded[] = $this->_adminData;
        }
        if ($this->_currentData != null) {
            $loaded[] = $this->_currentData;
        }
        if ($this->_specialData != null) {
            $loaded[] = $this->_specialData;
        }

        return $loaded;
    }
}
