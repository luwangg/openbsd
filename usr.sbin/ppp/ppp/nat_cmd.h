/*-
 * The code in this file was written by Eivind Eklund <perhaps@yes.no>,
 * who places it in the public domain without restriction.
 *
 *	$OpenBSD: src/usr.sbin/ppp/ppp/nat_cmd.h,v 1.3 2000/03/31 14:32:51 brian Exp $
 */

struct cmdargs;

extern int nat_RedirectPort(struct cmdargs const *);
extern int nat_RedirectAddr(struct cmdargs const *);
extern int nat_ProxyRule(struct cmdargs const *);
extern int nat_Pptp(struct cmdargs const *);
extern int nat_SetTarget(struct cmdargs const *);

extern struct layer natlayer;
