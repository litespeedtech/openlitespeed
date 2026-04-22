<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\UI\DTbl;

class PasswordPostTableValidationRule implements PostTableValidationRuleInterface
{
    public function Supports($request, $table)
    {
        $tid = $table->Get(DTbl::FLD_ID);
        if ($tid == 'V_UDB') {
            return true;
        }

        return ($request->GetView() == 'admin' && ($tid == 'ADM_USR' || $tid == 'ADM_USR_NEW'));
    }

    public function Validate($request, $extracted)
    {
        $tid = $request->GetTid();
        if ($tid == 'ADM_USR') {
            return $this->validateCurrentAdminUser($request, $extracted);
        }

        return $this->validateNewPassword($extracted);
    }

    private function validateCurrentAdminUser($request, $extracted)
    {
        $isValid = 1;
        $oldpass = $extracted->GetChildVal('oldpass');
        if ($oldpass == null) {
            $this->setChildErr($extracted, 'oldpass', 'Missing Old password!');
            $isValid = -1;
        } else {
            $udb = $request->GetConfData();
            $oldusername = $request->GetCurrentRef();
            $oldhash = $udb->GetChildVal('*index$name:passwd', $oldusername);

            if ($oldhash == null || !password_verify($oldpass, $oldhash)) {
                $this->setChildErr($extracted, 'oldpass', 'Invalid old password!');
                $isValid = -1;
            }
        }

        if ($this->validateNewPasswordFields($extracted) == -1) {
            $isValid = -1;
        }

        if ($isValid == -1) {
            return -1;
        }

        $extracted->AddChild(new CNode('passwd', $this->encryptPass($extracted->GetChildVal('pass'))));
        return 1;
    }

    private function validateNewPassword($extracted)
    {
        if ($this->validateNewPasswordFields($extracted) == -1) {
            return -1;
        }

        $extracted->AddChild(new CNode('passwd', $this->encryptPass($extracted->GetChildVal('pass'))));
        return 1;
    }

    private function validateNewPasswordFields($extracted)
    {
        $isValid = 1;
        $pass = $extracted->GetChildVal('pass');
        if ($pass == null) {
            $this->setChildErr($extracted, 'pass', 'Missing new password!');
            $isValid = -1;
        } elseif ($pass != $extracted->GetChildVal('pass1')) {
            $this->setChildErr($extracted, 'pass', 'New passwords do not match!');
            $isValid = -1;
        }

        return $isValid;
    }

    private function encryptPass($value)
    {
        return password_hash($value, PASSWORD_BCRYPT);
    }

    private function setChildErr($extracted, $key, $err)
    {
        if ($extracted->GetChildren($key) == null) {
            $extracted->AddChild(new CNode($key, ''));
        }

        $extracted->SetChildErr($key, $err);
    }
}
