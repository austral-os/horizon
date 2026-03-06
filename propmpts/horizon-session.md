Vamos a planificar como integrar horizon_session.
La idea final es que todas las aplicaciones sean clientes de session_manager y mediante el se comuniquen entre si.

actualmente algunas aplicaciones estan actuando como cliente y servidor al mismo tiempo. eso no me gusta.

Flujo actual:
Una aplicacion se conecta con el top_panel directamente y le envia el menu. cuando se hace click en un menu, el top_panel se comunica con horizon_menu_manager_r y le dice desplega tal menu. Cuando una opcion del menu es clickeada se le avisa a top_panel que se clickeo en tal opcion para que reaccione.

Esta comunicacion sucede con varios servidores/clientes para lograr ese resultado.

Flujo esperado:
Una aplicacion envia un mensaje al session_manager especificando como destinatario al top_panel. en ese mensaje se especifica quien es el remitence y quien es el receptor. ademas del mensaje prpiamente dicho.

session_manager recibe el mensaje y se lo envia al destinatario.

el destinatario, en este caso, el top_panel, recibe el mensaje con el menu y lo muestra.

cuando se activa algun menu mediante su titulo, el top_panel se comunica con session_manager enviando un mensaje cuyo destinatario sera el horizon_menu_manager quien debe mostrar el menu solicitado.

cuando se hace click en alguna opcion el menu se envia un mensaje desde el horizon_menu_manager a el top_panel. cuando el top_panel recibe en que menu se hizo click, decide si procesarlo el o si le envia un mensaje, por intermedio de session_manager a la aplicacion duania del menu para que procese el evento.

esta misma logica deberian usar otras aplicaciones como por ejemplo el dock. si se hace click en el icono de una aplicacion el dock envia un mensaje al session_manager pidiendole que maximice/minimice/o cierre una aplicacion.

session_manager recibe la solicitud y si corresponde la ejecuta el mismo o se la envia a la aplicacion para que la ejecute.