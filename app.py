import streamlit as st
import os
import json
from datetime import datetime

DATA_FILE = "bank_data.json"
TRANS_FILE = "transactions_data.json"
MIN_BALANCE = 500.0
ADMIN_PIN = "9999"  # Default Admin PIN for management tasks

# --- DATA HANDLING FUNCTIONS ---
def load_accounts():
    if not os.path.exists(DATA_FILE):
        return {}
    with open(DATA_FILE, "r") as f:
        return json.load(f)

def save_accounts(data):
    with open(DATA_FILE, "w") as f:
        json.dump(data, f, indent=4)

def load_transactions():
    if not os.path.exists(TRANS_FILE):
        return []
    with open(TRANS_FILE, "r") as f:
        return json.load(f)

def log_transaction(acc_no, t_type, amount, balance, details):
    trans = load_transactions()
    trans.append({
        "timestamp": datetime.now().strftime("%d-%m-%Y %H:%M:%S"),
        "acc_no": str(acc_no),
        "type": t_type,
        "amount": amount,
        "balance": balance,
        "details": details
    })
    with open(TRANS_FILE, "w") as f:
        json.dump(trans, f, indent=4)

def generate_acc_no(accounts):
    if not accounts:
        return "1001"
    existing_nums = [int(k) for k in accounts.keys()]
    return str(max(existing_nums) + 1)

# --- PAGE SETUP ---
st.set_page_config(page_title="NovaBank Management System", page_icon="💳", layout="wide")

st.title("💳 NovaBank Management System")
st.markdown("---")

accounts = load_accounts()

# --- SIDEBAR NAVIGATION ---
st.sidebar.header("Navigation Panel")
portal = st.sidebar.radio("Select Portal", ["Customer Portal", "Open New Account", "Admin Panel"])

# ==========================================
# 1. CUSTOMER PORTAL
# ==========================================
if portal == "Customer Portal":
    st.subheader("👤 Customer Login & Dashboard")
    
    col1, col2 = st.columns(2)
    with col1:
        login_acc = st.text_input("Account Number")
    with col2:
        login_pin = st.text_input("4-digit PIN", type="password")
        
    if st.button("Login"):
        if login_acc in accounts and accounts[login_acc]["pin"] == login_pin:
            st.session_state["logged_in_acc"] = login_acc
            st.success(f"Welcome back, {accounts[login_acc]['name']}!")
        else:
            st.error("Invalid Account Number or PIN.")

    if "logged_in_acc" in st.session_state and st.session_state["logged_in_acc"] in accounts:
        acc_no = st.session_state["logged_in_acc"]
        acc = accounts[acc_no]
        
        st.markdown(f"### Account Dashboard: A/C #{acc_no} ({acc['name']})")
        st.info(f"**Account Type:** {acc['type']} | **Current Balance:** Rs. {acc['balance']:.2f}")
        
        tab1, tab2, tab3, tab4, tab5 = st.tabs(["Balance Enquiry", "Deposit", "Withdraw", "Transfer", "Transaction History"])
        
        with tab1:
            st.write(f"Your current balance is: **Rs. {acc['balance']:.2f}**")
            
        with tab2:
            dep_amount = st.number_input("Amount to Deposit", min_value=1.0, key="dep")
            if st.button("Confirm Deposit"):
                accounts[acc_no]["balance"] += dep_amount
                save_accounts(accounts)
                log_transaction(acc_no, "DEPOSIT", dep_amount, accounts[acc_no]["balance"], "Cash Deposit")
                st.success(f"Successfully deposited Rs. {dep_amount:.2f}. New Balance: Rs. {accounts[acc_no]['balance']:.2f}")
                st.rerun()
                
        with tab3:
            wit_amount = st.number_input("Amount to Withdraw", min_value=1.0, key="wit")
            if st.button("Confirm Withdrawal"):
                if accounts[acc_no]["balance"] - wit_amount < MIN_BALANCE:
                    st.error(f"Insufficient balance! Minimum maintenance balance of Rs. {MIN_BALANCE} required.")
                else:
                    accounts[acc_no]["balance"] -= wit_amount
                    save_accounts(accounts)
                    log_transaction(acc_no, "WITHDRAW", wit_amount, accounts[acc_no]["balance"], "Cash Withdrawal")
                    st.success(f"Successfully withdrawn Rs. {wit_amount:.2f}. New Balance: Rs. {accounts[acc_no]['balance']:.2f}")
                    st.rerun()
                    
        with tab4:
            target_acc = st.text_input("Recipient Account Number")
            trans_amount = st.number_input("Amount to Transfer", min_value=1.0, key="tr_amt")
            if st.button("Transfer Funds"):
                if target_acc not in accounts:
                    st.error("Recipient account not found.")
                elif target_acc == acc_no:
                    st.error("Cannot transfer to your own account.")
                elif accounts[acc_no]["balance"] - trans_amount < MIN_BALANCE:
                    st.error("Insufficient balance for transfer.")
                else:
                    accounts[acc_no]["balance"] -= trans_amount
                    accounts[target_acc]["balance"] += trans_amount
                    save_accounts(accounts)
                    log_transaction(acc_no, "TRANSFER-OUT", trans_amount, accounts[acc_no]["balance"], f"Transfer to {target_acc}")
                    log_transaction(target_acc, "TRANSFER-IN", trans_amount, accounts[target_acc]["balance"], f"Transfer from {acc_no}")
                    st.success(f"Transferred Rs. {trans_amount:.2f} to A/C {target_acc} successfully!")
                    st.rerun()
                    
        with tab5:
            st.markdown("#### Your Transaction Logs")
            all_trans = load_transactions()
            user_trans = [t for t in all_trans if t["acc_no"] == acc_no]
            if user_trans:
                for t in reversed(user_trans):
                    st.text(f"[{t['timestamp']}] {t['type']} | Amount: Rs.{t['amount']:.2f} | Bal: Rs.{t['balance']:.2f} | {t['details']}")
            else:
                st.info("No transaction history available.")
                
        if st.button("Logout"):
            del st.session_state["logged_in_acc"]
            st.rerun()

# ==========================================
# 2. OPEN NEW ACCOUNT
# ==========================================
elif portal == "Open New Account":
    st.subheader("📝 Open a New Bank Account")
    
    with st.form("create_acc_form"):
        name = st.text_input("Account Holder Full Name")
        acc_type = st.selectbox("Account Type", ["Savings", "Current"])
        pin = st.text_input("Set 4-digit PIN", type="password", max_chars=4)
        deposit = st.number_input(f"Initial Opening Deposit (Min Rs. {MIN_BALANCE})", min_value=MIN_BALANCE, value=MIN_BALANCE)
        
        submit = st.form_submit_button("Create Account")
        
        if submit:
            if not name.strip():
                st.error("Please enter a valid name.")
            elif len(pin) != 4 or not pin.isdigit():
                st.error("PIN must be exactly 4 digits.")
            else:
                new_no = generate_acc_no(accounts)
                accounts[new_no] = {
                    "name": name,
                    "type": acc_type,
                    "pin": pin,
                    "balance": deposit,
                    "created": datetime.now().strftime("%d-%m-%Y")
                }
                save_accounts(accounts)
                log_transaction(new_no, "OPEN", deposit, deposit, "Account Opened")
                st.success(f"Account successfully created! Your assigned **Account Number is: {new_no}**")

# ==========================================
# 3. ADMIN PANEL
# ==========================================
elif portal == "Admin Panel":
    st.subheader("🛡️ Bank Administrator Portal")
    
    admin_entered_pin = st.text_input("Enter Admin Security PIN", type="password")
    
    if admin_entered_pin == ADMIN_PIN:
        st.success("Admin Authentication Successful!")
        
        admin_tab1, admin_tab2, admin_tab3 = st.tabs(["All Accounts", "Manage / Delete Account", "System Audit Logs"])
        
        with admin_tab1:
            st.markdown("### Registered Bank Accounts")
            if accounts:
                total_deposits = 0
                acc_list = []
                for acc_no, details in accounts.items():
                    acc_list.append({
                        "Acc No": acc_no,
                        "Name": details["name"],
                        "Type": details["type"],
                        "Balance (Rs.)": f"{details['balance']:.2f}",
                        "Created": details.get("created", "N/A")
                    })
                    total_deposits += details["balance"]
                st.table(acc_list)
                st.metric(label="Total Bank Assets (Deposits)", value=f"Rs. {total_deposits:.2f}")
                st.metric(label="Total Active Accounts", value=len(accounts))
            else:
                st.info("No accounts currently registered in the system.")
                
        with admin_tab2:
            st.markdown("### Delete or Modify Account")
            target_del = st.text_input("Enter Account Number to Manage")
            if target_del in accounts:
                st.write(f"**Holder Name:** {accounts[target_del]['name']} | **Balance:** Rs. {accounts[target_del]['balance']:.2f}")
                if st.button("Delete This Account", type="primary"):
                    del accounts[target_del]
                    save_accounts(accounts)
                    log_transactions_list = load_transactions()
                    log_transactions_list.append({
                        "timestamp": datetime.now().strftime("%d-%m-%Y %H:%M:%S"),
                        "acc_no": target_del,
                        "type": "DELETE",
                        "amount": 0,
                        "balance": 0,
                        "details": "Account deleted by Admin"
                    })
                    with open(TRANS_FILE, "w") as f:
                        json.dump(log_transactions_list, f, indent=4)
                    st.success(f"Account {target_del} deleted successfully.")
                    st.rerun()
            elif target_del:
                st.error("Account not found.")
                
        with admin_tab3:
            st.markdown("### Full Bank System Transaction Logs")
            all_trans = load_transactions()
            if all_trans:
                for t in reversed(all_trans):
                    st.text(f"[{t['timestamp']}] A/C: {t['acc_no']} | {t['type']} | Amt: Rs.{t['amount']:.2f} | Bal: Rs.{t['balance']:.2f} | {t['details']}")
            else:
                st.info("No system transactions recorded yet.")
                
    elif admin_entered_pin != "":
        st.error("Incorrect Admin PIN. (Default test PIN is 9999).")
