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
	"html/template"
	"io/ioutil"
	"log"
	"net/http"
	"regexp"
)

type GenData struct {
	LSRECAPTCHA_SITE_KEY   string
	LSRECAPTCHA_TYPE_PARAM template.JS
}

const (
	regexpSub        = "(?:<!--#echo var=\"ENV\\:(.*?)\"-->)"
	regexpRepl       = "{{.$1}}"
	envNameSiteKey   = "LSRECAPTCHA_SITE_KEY"
	envNameTypeParam = "LSRECAPTCHA_TYPE_PARAM"
)

var genRegex *regexp.Regexp

func initGenerator() {
	genRegex = regexp.MustCompile(regexpSub)
}

func validateTypeParam(s string) template.JS {
	// NOTE: This is typically unsafe. However, the string is expected to be
	// extra parameters to the recaptcha render function. Currently,
	// they are hardcoded values in the calling server. If that changes, how
	// safe the parameter is should be taken into consideration.

	return template.JS(s)
}

func generateForm(w http.ResponseWriter, r *http.Request) {
	reqEnvs := getReqEnv(r)
	if reqEnvs == nil {
		log.Println("missing request environments")
		return
	}
	sitekey := reqEnvs[envNameSiteKey]
	typeparam := validateTypeParam(reqEnvs[envNameTypeParam])

	d := &GenData{LSRECAPTCHA_SITE_KEY: sitekey, LSRECAPTCHA_TYPE_PARAM: typeparam}

	fileBytes, _ := ioutil.ReadFile("_recaptcha.shtml")

	f := genRegex.ReplaceAll(fileBytes, []byte(regexpRepl))

	t := template.New("recaptcha")
	t, err := t.Parse(string(f))
	if err != nil {
		w.WriteHeader(404)
	}
	t.Execute(w, d)
}
