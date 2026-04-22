<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

function lstServiceRequestOrFail($requestType, $actId = '')
{
    if (Service::ServiceRequest($requestType, $actId) !== false) {
        return;
    }

    http_response_code(500);
    header('Content-Type: text/plain; charset=UTF-8');

    $message = trim((string) Service::GetLastServiceRequestError());
    if ($message === '') {
        $message = DMsg::UIStr('service_requestfailed');
    }

    echo $message;
    exit;
}

$requestMethod = isset($_SERVER['REQUEST_METHOD']) ? strtoupper((string) $_SERVER['REQUEST_METHOD']) : '';
if ($requestMethod !== 'POST') {
    http_response_code(405);
    header('Allow: POST');
    exit;
}

$act = UIBase::GrabGoodInput("post",'act');
$actId = UIBase::GrabGoodInput("post",'actId');

switch ($act) {
	case 'restart':
		lstServiceRequestOrFail(SInfo::SREQ_RESTART_SERVER);
		break;

	case 'lang':
		DMsg::SetLang($actId);
		break;

	case 'toggledebug':
		lstServiceRequestOrFail(SInfo::SREQ_TOGGLE_DEBUG);
		break;

	case 'reload':
		if ($actId != '') {
			lstServiceRequestOrFail(SInfo::SREQ_VH_RELOAD, $actId);
		}
		break;
	case 'disable':
		if ($actId != '') {
			lstServiceRequestOrFail(SInfo::SREQ_VH_DISABLE, $actId);
		}
		break;
	case 'enable':
		if ($actId != '') {
			lstServiceRequestOrFail(SInfo::SREQ_VH_ENABLE, $actId);
		}
		break;
	default:
		error_log("illegal act $act ");
}
