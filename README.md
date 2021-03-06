A Programming Interview Question
--------------------------------

Given a list of at most N = 50,000,000 records (in CSV format), each record describes an usage
period of a specific mobile phone number.
Note that one phone number can occurs multiple times in this list, because of 2 reasons:
- This phone number can change from prepaid plan to postpaid plan, or vice versa, at
anytime just by sending an SMS to the operator.
- Or, the owner of this phone number can stop using it, and after 1-2 months, it is reused
by another person.
Also remember that, the reason is not recorded in the data, we just have the phone number and
its activation or deactivation date for a usage period record.
- Activation date is the date that the phone number is started being used by a owner with
a specific plan (prepaid or postpaid).
- Deactivation date is the date that the phone number is stopped being used by a owner
with the registered plan.
Moreover, the records don't need to follow any specific order of time, and the records of
the same number don't need to be consecutive.
This is an example of the list, given as a CSV file:

    PHONE_NUMBER,ACTIVATION_DATE,DEACTIVATION_DATE
    0987000001,2016-03-01,2016-05-01
    0987000002,2016-02-01,2016-03-01
    0987000001,2016-01-01,2016-03-01
    0987000001,2016-12-01,
    0987000002,2016-03-01,2016-05-01
    0987000003,2016-01-01,2016-01-10
    0987000001,2016-09-01,2016-12-01
    0987000002,2016-05-01,
    0987000001,2016-06-01,2016-09-01

In this list, ACTIVATION_DATE and DEACTIVATION_DATE are represented with
YYYY-MM-DD format. DEACTIVATION_DATE may be empty, which means that the phone is
still being used by its current owner.
From the given data, we want to find a list of unique phone numbers together with the actual
activation date when its current owner started using it. Note that what we need is the first
activation date of current owner, not previous owner, and not the date when current owner
changes prepaid/postpaid plans.
For example: The prepaid phone number 0987000001 was used by A from 2016-01-01 to
2016-03-01, then it was changed to postpaid. A continued using it until 2016-05-01 and
stopped using this number. After 1 month, on 2016-06-01, this phone number was reused by B
with prepaid plan. B used it until 2016-09-01 then changed to postpaid, and finally changed
back to prepaid on 2016-12-01 and he's still using it until now. In this case, the actual activation
date of current owner B of 0987000001 that we want to find is 2016-06-01.