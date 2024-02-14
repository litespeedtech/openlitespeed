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

// Package main is a Web server used for handling recaptcha results.
package main

import (
	"log"
	"ls/lsapi"
	"net/http"
	"net/url"
)

func router(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case http.MethodGet:
		generateForm(w, r)
		return
	case http.MethodPost:
		verifyResp(w, r)
	default:
	}
	ref := "/"

	refHeader := r.Header.Get("Referer")

	r.URL.Query()

	if refHeader != "" {
		u, err := url.Parse(refHeader)
		if err != nil {
			log.Printf("Failed to parse referer %+v", err)
		} else {
			ref = u.EscapedPath()
		}
	}

	if r.URL.RawQuery != "" {
		ref += "?" + r.URL.RawQuery
	}
	w.Header().Add("Location", ref)
	w.WriteHeader(301)
}

func getReqEnv(r *http.Request) map[string]string {
	return lsapi.GetReqEnv(r.Context())
}

func main() {
	initGenerator()
	initVerifier()
	lsapi.LogToFile("")
	lsapi.Init("", "")

	http.HandleFunc("/", router)
	err := lsapi.ListenAndServe("uds://tmp/lsrecaptcha.sock", nil)
	if err != nil {
		log.Printf("listen and serve failed, %+v\n", err)
	}
}
