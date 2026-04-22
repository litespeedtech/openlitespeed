<?php

namespace LSWebAdmin\UI;

class ConfPageResolver
{
    public static function resolve($routeOrDisplay, $pageDefClass)
    {
        $pageDef = call_user_func([$pageDefClass, 'GetInstance']);
        list($view, $pid) = self::resolvePageIdentity($routeOrDisplay);

        if (method_exists($pageDef, 'ResolvePage')) {
            return $pageDef->ResolvePage($view, $pid);
        }

        return null;
    }

    private static function resolvePageIdentity($routeOrDisplay)
    {
        if ($routeOrDisplay instanceof ConfRouteState) {
            return [$routeOrDisplay->GetView(), $routeOrDisplay->GetPid()];
        }

        if (is_object($routeOrDisplay) && method_exists($routeOrDisplay, 'GetRouteState')) {
            $routeState = $routeOrDisplay->GetRouteState();
            return [$routeState->GetView(), $routeState->GetPid()];
        }

        if (is_object($routeOrDisplay)
            && method_exists($routeOrDisplay, 'GetView')
            && method_exists($routeOrDisplay, 'GetPid')) {
            return [$routeOrDisplay->GetView(), $routeOrDisplay->GetPid()];
        }

        return ['', ''];
    }
}
