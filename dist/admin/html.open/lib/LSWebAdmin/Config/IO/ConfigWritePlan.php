<?php

namespace LSWebAdmin\Config\IO;

class ConfigWritePlan
{
    private $_tasks = [];

    public function AddTask($task)
    {
        $this->_tasks[] = $task;
    }

    public function GetTasks()
    {
        return $this->_tasks;
    }
}
