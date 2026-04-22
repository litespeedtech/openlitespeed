<?php

namespace LSWebAdmin\Runtime;

class ServiceCommandPlanner
{
    public static function plan($type, $actId = '')
    {
        switch ($type) {
            case SInfo::SREQ_RESTART_SERVER:
                return new ServiceCommandPlan('restart', true);

            case SInfo::SREQ_TOGGLE_DEBUG:
                return new ServiceCommandPlan('toggledbg');

            case SInfo::SREQ_VH_RELOAD:
                return new ServiceCommandPlan('restart:vhost:' . $actId);

            case SInfo::SREQ_VH_DISABLE:
                return new ServiceCommandPlan('disable:vhost:' . $actId, false, $type);

            case SInfo::SREQ_VH_ENABLE:
                return new ServiceCommandPlan('enable:vhost:' . $actId, false, $type);
        }

        return null;
    }
}
