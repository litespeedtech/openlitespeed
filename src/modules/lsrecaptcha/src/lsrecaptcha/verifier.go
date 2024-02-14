//****************************************************************************
//   Open LiteSpeed is an open source HTTP server.                           *
//   Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
//                                                                           *
//   This program is free software: you can redistribute it and/or modify    *
//   it under the terms of the GNU General Public License as published by    *
//   the Free Software Foundation, either version 3 of the License, or       *
//   (at your option) any later version.                                     *
//                                                                           *
//   This program is distributed in the hope that it will be useful,         *
//   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
//   GNU General Public License for more details.                            *
//                                                                           *
//   You should have received a copy of the GNU General Public License       *
//   along with this program. If not, see http://www.gnu.org/licenses/.      *
//****************************************************************************

package main

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"time"
)

type RecaptchaResponse struct {
	Success     bool      `json:"success"`
	ChallengeTS time.Time `json:"challenge_ts"`
	Hostname    string    `json:"hostname"`
	ErrorCodes  []string  `json:"error-codes"`
}

const (
	recaptchaServerName = "https://www.google.com/recaptcha/api/siteverify"
	recaptchaFormName   = "g-recaptcha-response"
	envNameSecretKey    = "LSRECAPTCHA_SECRET_KEY"
)

func initVerifier() {
}


func verifyResp(w http.ResponseWriter, r *http.Request) {

	err := r.ParseForm()
	if err != nil {
		log.Printf("Failed to parse form, %+v", err)
		return
	}

	reqEnvs := getReqEnv(r)
	if reqEnvs == nil {
		log.Println("missing request environments")
		return
	}

	ip := reqEnvs["REMOTE_ADDR"]
	secretKey := reqEnvs[envNameSecretKey]
	js_obj_name := reqEnvs["LSRECAPTCHA_JS_OBJ_NAME"];

	var api_url = recaptchaServerName
	var form_var_name string

	if js_obj_name == "hcaptcha" {
		form_var_name = "h-captcha-response";
		api_url = "https://api.hcaptcha.com/siteverify"
	} else {
		form_var_name = recaptchaFormName;
		api_url = recaptchaServerName;
	}

	val := r.PostFormValue(form_var_name)
	if (val == "" ) {
		log.Printf("POST form value %s is blank", form_var_name)
		return;
	}

	reqVars := url.Values{"secret": {secretKey},
			"remoteip": {ip},
			"response": {val}}

	resp, err := http.PostForm(api_url, reqVars)


	if err != nil {
		log.Printf("Failed to post form. %+v", err)
		return
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)

	if err != nil {
		log.Printf("Failed to extract Google's response. %+v", err)
		return
	}

	var recaptcha RecaptchaResponse

	err = json.Unmarshal(body, &recaptcha)

	if err != nil {
		log.Printf("Google's response is malformed. %+v", err)
		return
	}

	if !recaptcha.Success {
		log.Printf("Recaptcha not ok, errors %+v", recaptcha.ErrorCodes)
		return
	}

	w.Header().Add("lsrecaptcha", "1")
}
