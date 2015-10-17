join_gen perm_HEAD_S_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(s, s)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_S_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(s, o)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_S_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(s, &locals.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_S_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(s, &consts.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_O_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(o, s)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_O_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(o, o)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_O_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(o, &locals.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_HEAD_O_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(o, &consts.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_LOCAL_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&locals.at(wi), s)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_LOCAL_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&locals.at(wi), o)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_LOCAL_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&locals.at(wi), &locals.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_LOCAL_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&locals.at(wi), &consts.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_CONST_HEAD_S(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&consts.at(wi), s)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_CONST_HEAD_O(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&consts.at(wi), o)) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_CONST_LOCAL(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&consts.at(wi), &locals.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}
join_gen perm_CONST_CONST(nodeid a, join_gen b, size_t wi, size_t xi, Locals &consts)
{
	FUN;
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a, b, wi, xi, entry, round, ac, bc, &consts]()mutable {
		setproc(L"join gen");
		return [a, b, wi, xi, entry, round, ac, bc, &consts](Thing *s, Thing *o, Locals &locals)mutable {
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch (entry) {
				case 0:
					//TRACE( dout << sprintPred(L"a()",a) << endl;)
					ac = preds.at(a);
					while (ac(&consts.at(wi), &consts.at(xi))) {
						TRACE(dout << "MATCH A." << endl;)
						bc = b();
						while (bc(s, o, locals)) {
							TRACE(dout << "MATCH." << endl;)
							entry = 1;
							return true;
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
					entry = 666;
					TRACE(dout << "DONE." << endl;)
					return false;
				default:
					assert(false);
			}
		};
	};
}

void make_perms()
{
permname[HEAD_S] = L"HEAD_S";
permname[HEAD_O] = L"HEAD_O";
permname[LOCAL] = L"LOCAL";
permname[CONST] = L"CONST";
perms[HEAD_S][HEAD_S] = perm_HEAD_S_HEAD_S;
perms[HEAD_S][HEAD_O] = perm_HEAD_S_HEAD_O;
perms[HEAD_S][LOCAL] = perm_HEAD_S_LOCAL;
perms[HEAD_S][CONST] = perm_HEAD_S_CONST;
perms[HEAD_O][HEAD_S] = perm_HEAD_O_HEAD_S;
perms[HEAD_O][HEAD_O] = perm_HEAD_O_HEAD_O;
perms[HEAD_O][LOCAL] = perm_HEAD_O_LOCAL;
perms[HEAD_O][CONST] = perm_HEAD_O_CONST;
perms[LOCAL][HEAD_S] = perm_LOCAL_HEAD_S;
perms[LOCAL][HEAD_O] = perm_LOCAL_HEAD_O;
perms[LOCAL][LOCAL] = perm_LOCAL_LOCAL;
perms[LOCAL][CONST] = perm_LOCAL_CONST;
perms[CONST][HEAD_S] = perm_CONST_HEAD_S;
perms[CONST][HEAD_O] = perm_CONST_HEAD_O;
perms[CONST][LOCAL] = perm_CONST_LOCAL;
perms[CONST][CONST] = perm_CONST_CONST;
}
