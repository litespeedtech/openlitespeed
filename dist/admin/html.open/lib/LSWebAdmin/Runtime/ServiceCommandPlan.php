<?php

namespace LSWebAdmin\Runtime;

class ServiceCommandPlan
{
    private $_command;
    private $_resetChanged;
    private $_suspendedVhostAction;

    public function __construct($command, $resetChanged = false, $suspendedVhostAction = null)
    {
        $this->_command = $command;
        $this->_resetChanged = (bool) $resetChanged;
        $this->_suspendedVhostAction = $suspendedVhostAction;
    }

    public function GetCommand()
    {
        return $this->_command;
    }

    public function ShouldResetChanged()
    {
        return $this->_resetChanged;
    }

    public function GetSuspendedVhostAction()
    {
        return $this->_suspendedVhostAction;
    }

    public function RequiresServerConfigMutation()
    {
        return ($this->_suspendedVhostAction != null);
    }
}
