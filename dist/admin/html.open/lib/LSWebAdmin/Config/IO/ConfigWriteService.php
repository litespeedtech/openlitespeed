<?php

namespace LSWebAdmin\Config\IO;

class ConfigWriteService
{
    public static function execute($plan)
    {
        $lastRoot = null;

        foreach ($plan->GetTasks() as $task) {
            $root = $task->UsesPreviousRoot() ? $lastRoot : $task->GetRoot();

            switch ($task->GetMode()) {
                case ConfigWriteTask::MODE_SAVE_ROOT:
                    $lastRoot = ConfigDataWriter::saveMappedRoot(
                        $task->GetFormat(),
                        $task->GetType(),
                        $root,
                        $task->GetFileMap(),
                        $task->GetFilePath(),
                        $task->GetPermissionSource()
                    );
                    break;

                case ConfigWriteTask::MODE_WRITE_ROOT:
                    $lastRoot = ConfigDataWriter::writeRoot(
                        $task->GetFormat(),
                        $task->GetType(),
                        $root,
                        $task->GetFilePath(),
                        $task->GetPermissionSource()
                    );
                    break;
            }
        }

        return $lastRoot;
    }
}
