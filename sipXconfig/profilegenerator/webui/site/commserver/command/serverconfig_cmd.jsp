<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<%
    // Get the server name.
    String servername = request.getParameter("servername");
%>

<html>
<head>
	<title>Untitled</title>
<link rel="stylesheet" href="../../style/dms.css" type="text/css">
<script src="../script/jsFunctions.js"></script>
</head>

<body class="bglight" onLoad="MM_preloadImages('../../ui/command/buttons/save_btn_f2.gif','../../ui/command/buttons/cancel_btn_f2.gif')">
<table cellspacing="1" cellpadding="4" border="0">
    <tr>
        <td><a href="#" onMouseOut="MM_swapImgRestore()"  onMouseOver="MM_swapImage('save_btn','','../../ui/command/buttons/save_btn_f2.gif',1);" onFocus="if(this.blur)this.blur()" onClick="submitform()"><img name="save_btn" src="../../ui/command/buttons/save_btn.gif" border="0"></a></td>
        <td><a href="../../ui/administration/configure_services.jsp" target="mainFrame" onMouseOut="MM_swapImgRestore()"  onMouseOver="MM_swapImage('cancel_btn','','../../ui/command/buttons/cancel_btn_f2.gif',1);" onFocus="if(this.blur)this.blur()"><img name="cancel_btn" src="../../ui/command/buttons/cancel_btn.gif" border="0"></a></td>
        <td><a href="javascript:void 0" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('defaults_btn','','../images/restore_to_defaults_btn_f2.gif',1);" onFocus="if(this.blur)this.blur()" onClick="restoreToDefaults('/pds/commserver/serverconfig_save.jsp?restore=true&servername=<%=servername%>')"><img name="defaults_btn" src="../images/restore_to_defaults_btn.gif" border="0"></a></td>
    </tr>
</table>
</body>
</html>
