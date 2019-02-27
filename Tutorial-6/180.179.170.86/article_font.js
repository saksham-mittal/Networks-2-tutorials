HTTP/1.1 200 OK
Date: Wed, 27 Feb 2019 19:46:35 GMT
Content-Type: application/x-javascript
Content-Length: 1701
Last-Modified: Mon, 24 Mar 2014 07:49:02 GMT
Connection: keep-alive
Vary: Accept-Encoding
ETag: "532fe36e-6a5"
Server: BJP Server (qmpl-vm4)
Expires: Thu, 31 Dec 2037 23:55:55 GMT
Cache-Control: max-age=315360000
Cache-Control: public
Accept-Ranges: bytes

/*
Plugin Article PXFont Size for Joomla! v2.5
Copyright (C) 2012 (http://www.karmany.net)
License GNU/GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
	
Please retain this copyright notice.
You can use this code if this copyright notice are included
*/

//Los tags que seran modificados estan en el array tgs, que es pasado por php
var sz = new Array();
var sz_inicial = 10;
var sz_maximo = 10;
var sz_minimo = 10;
var formato = "px";
var sz_inc = 1;

function init_common_datos (inicial,maximo,minimo,txt_format,inc)
{
	//Inicializa todos los datos comunes
	sz_inicial = inicial;
	sz_maximo = maximo;
	sz_minimo = minimo;
	formato = txt_format;
	sz_inc = inc;	
}
function init_individual_datos (contador)
{
	//Inicializa solo los datos individuales
	sz[contador] = sz_inicial;
}

function modify_size(busca,inc,contador) {
	
	var doc = document,eldoc = null,busca_tgs,i,j;

	switch (inc)
	{
		case 0: //Reset
			sz[contador] = sz_inicial;
			break;
		case 