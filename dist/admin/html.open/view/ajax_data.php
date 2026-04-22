<?php

use LSWebAdmin\Ajax\BlockedIpsAjax;
use LSWebAdmin\Ajax\BuildAjax;
use LSWebAdmin\Ajax\DashboardAjax;
use LSWebAdmin\Ajax\ListenerVhMapAjax;
use LSWebAdmin\Ajax\LogViewerAjax;
use LSWebAdmin\Ajax\RealtimeStatsAjax;
use LSWebAdmin\UI\UIBase;

$requestId = isset($_GET['id']) && is_scalar($_GET['id']) ? trim((string) $_GET['id']) : '';
if ($requestId == 'pid_load') {
    define('NO_UPDATE_ACCESS', 1);
}

require_once __DIR__ . '/inc/auth.php';

$routes = [
    'dashsummary' => [DashboardAjax::class, 'dashSummary'],
    'dashstat' => [DashboardAjax::class, 'dashStat'],
    'dashstatus' => [DashboardAjax::class, 'dashStatus'],
    'dashlog' => [DashboardAjax::class, 'dashLog'],
    'pid_load' => [DashboardAjax::class, 'pidLoad'],
    'plotstat' => [RealtimeStatsAjax::class, 'plotStat'],
    'vhstat' => [RealtimeStatsAjax::class, 'vhStat'],
    'viewlog' => [LogViewerAjax::class, 'viewLog'],
    'searchlog' => [LogViewerAjax::class, 'searchLog'],
    'downloadlog' => [LogViewerAjax::class, 'downloadLog'],
    'buildprogress' => [BuildAjax::class, 'buildProgress'],
    'blockedipspreview' => [BlockedIpsAjax::class, 'preview'],
    'blockedipsfull' => [BlockedIpsAjax::class, 'full'],
    'downloadblockedips' => [BlockedIpsAjax::class, 'download'],
    'listenervhmapfull' => [ListenerVhMapAjax::class, 'full'],
];

$id = UIBase::GrabGoodInput('get', 'id');

if (isset($routes[$id])) {
    call_user_func($routes[$id]);
} else {
    error_log("invalid action ajax_data id = $id");
}
