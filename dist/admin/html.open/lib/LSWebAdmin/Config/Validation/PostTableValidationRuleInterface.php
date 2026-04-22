<?php

namespace LSWebAdmin\Config\Validation;

interface PostTableValidationRuleInterface
{
    public function Supports($request, $table);

    public function Validate($request, $extracted);
}
