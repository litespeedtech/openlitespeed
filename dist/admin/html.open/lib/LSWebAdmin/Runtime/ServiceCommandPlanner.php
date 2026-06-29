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
                if (!self::isSafeActionId($actId)) {
                    return null;
                }
                return new ServiceCommandPlan('restart:vhost:' . $actId);

            case SInfo::SREQ_VH_DISABLE:
                if (!self::isSafeActionId($actId)) {
                    return null;
                }
                return new ServiceCommandPlan('disable:vhost:' . $actId, false, $type);

            case SInfo::SREQ_VH_ENABLE:
                if (!self::isSafeActionId($actId)) {
                    return null;
                }
                return new ServiceCommandPlan('enable:vhost:' . $actId, false, $type);
        }

        return null;
    }

    private static function isSafeActionId($actId)
    {
        $actId = (string) $actId;
        if ($actId === '' || strlen($actId) > 100) {
            return false;
        }

        // Reject:
        //   \x00-\x1F, \x7F  control chars (CR/LF that could split the
        //                    admin socket command, etc.)
        //   :                splits the existing "restart:vhost:<id>"
        //                    command framing.
        //   , ;              the suspendedVhosts config value is stored
        //                    as a comma/semicolon-separated list and
        //                    re-split with preg_split("/[,;]+/", ...)
        //                    in SuspendedVhostMutationService — a vhost
        //                    name like "vh1,vh2" would otherwise be
        //                    stored as one entry and re-read as two
        //                    separate disabled vhosts, letting a single
        //                    POST disable several vhosts in one call.
        //   whitespace       conservative; vhost names with embedded
        //                    spaces aren't reachable through normal
        //                    config flows and could confuse downstream
        //                    parsers (admin socket, suspendedVhosts
        //                    list, audit log lines).
        return (preg_match('/[\x00-\x1F\x7F:,;\s]/', $actId) !== 1);
    }
}
