<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\Product\Current\DTblDef;
use LSWebAdmin\UI\ConfPageResolver;

class ConfigValidationRequest
{
    private $_tid;
    private $_table;
    private $_act;
    private $_view;
    private $_viewName;
    private $_currentRef;
    private $_parentRef;
    private $_confData;
    private $_vhRoot;
    private $_tableLocation;
    private $_derivedSelOptionsResolver;
    private $_inputSource;

    public function __construct(
        $tid,
        $table,
        $act,
        $view,
        $viewName,
        $currentRef,
        $parentRef,
        $confData,
        $vhRoot,
        $tableLocation,
        $derivedSelOptionsResolver = null,
        $inputSource = null
    ) {
        $this->_tid = $tid;
        $this->_table = $table;
        $this->_act = $act;
        $this->_view = $view;
        $this->_viewName = $viewName;
        $this->_currentRef = $currentRef;
        $this->_parentRef = $parentRef;
        $this->_confData = $confData;
        $this->_vhRoot = $vhRoot;
        $this->_tableLocation = $tableLocation;
        $this->_derivedSelOptionsResolver = $derivedSelOptionsResolver;
        $this->_inputSource = $inputSource;
    }

    public static function fromRouteState(
        $routeState,
        $confData,
        $vhRoot,
        $derivedSelOptionsResolver = null,
        $inputSource = null,
        $tableDefClass = null,
        $pageDefClass = null
    ) {
        if ($tableDefClass == null) {
            $tableDefClass = DTblDef::class;
        }
        if ($pageDefClass == null) {
            $pageDefClass = DPageDef::class;
        }

        $tid = $routeState->GetLastTid();
        $table = call_user_func([$tableDefClass, 'GetInstance'])->GetTblDef($tid);
        $page = ConfPageResolver::resolve($routeState, $pageDefClass);
        $tableLocation = $page->GetTblMap()->FindTblLoc($tid);

        return new self(
            $tid,
            $table,
            $routeState->GetAct(),
            $routeState->GetView(),
            $routeState->GetViewName(),
            $routeState->GetLastRef(),
            $routeState->GetParentRef(),
            $confData,
            $vhRoot,
            $tableLocation,
            $derivedSelOptionsResolver,
            $inputSource
        );
    }

    public function GetTid()
    {
        return $this->_tid;
    }

    public function GetTable()
    {
        return $this->_table;
    }

    public function GetAct()
    {
        return $this->_act;
    }

    public function GetView()
    {
        return $this->_view;
    }

    public function GetViewName()
    {
        return $this->_viewName;
    }

    public function GetCurrentRef()
    {
        return $this->_currentRef;
    }

    public function GetParentRef()
    {
        return $this->_parentRef;
    }

    public function GetConfData()
    {
        return $this->_confData;
    }

    public function GetVHRoot()
    {
        return $this->_vhRoot;
    }

    public function GetTableLocation()
    {
        return $this->_tableLocation;
    }

    public function ResolveDerivedSelOptions($loc, $node)
    {
        if (!is_callable($this->_derivedSelOptionsResolver)) {
            return [];
        }

        return call_user_func($this->_derivedSelOptionsResolver, $this->_tid, $loc, $node);
    }

    public function GetInputSource()
    {
        return $this->_inputSource;
    }
}
