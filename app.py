import streamlit as st
import os
import json

DATA_FILE = "accounts.json"

def load_data():
    if not os.path.exists(DATA_FILE):
        return {}
    with open(DATA_FILE, "r") as f:
        return json.load(f)

def save_data(data):
    with open(DATA_FILE, "w") as f:
        json.dump(data, f)

st.title("💳 NovaBank Management System")

accounts = load_data()

menu = ["Create Account", "Deposit", "Withdraw", "Balance Enquiry", "All Accounts"]
choice = st.sidebar.selectbox("Select Operation", menu)

if choice == "Create Account":
    st.subheader("Open a New Bank Account")
    name = st.text_input("Account Holder Name")
    acc_type = st.selectbox("Account Type", ["Savings", "Current"])
    pin = st.text_input("Set 4-digit PIN", type="password")
    deposit = st.number_input("Initial Deposit (Min 500)", min_value=500.0, value=500.0)
    
    if st.button("Create Account"):
        if name and len(pin) == 4:
            acc_no = str(int(max([int(k) for k in accounts.keys()] or [1000])) + 1)
            accounts[acc_no] = {"name": name, "type": acc_type, "pin": pin, "balance": deposit}
            save_data(accounts)
            st.success(f"Account created successfully! Your Account Number is: **{acc_no}**")
        else:
            st.error("Please enter a valid name and a 4-digit PIN.")

elif choice == "Balance Enquiry":
    st.subheader("Check Account Balance")
    acc_no = st.text_input("Account Number")
    pin = st.text_input("PIN", type="password")
    
    if st.button("Check"):
        if acc_no in accounts and accounts[acc_no]["pin"] == pin:
            st.info(f"Account Holder: {accounts[acc_no]['name']}")
            st.success(f"Current Balance: Rs. {accounts[acc_no]['balance']:.2f}")
        else:
            st.error("Invalid Account Number or PIN.")

elif choice == "Deposit":
    st.subheader("Deposit Money")
    acc_no = st.text_input("Account Number")
    amount = st.number_input("Amount to Deposit", min_value=1.0)
    
    if st.button("Deposit"):
        if acc_no in accounts:
            accounts[acc_no]["balance"] += amount
            save_data(accounts)
            st.success(f"Deposited successfully! New Balance: Rs. {accounts[acc_no]['balance']:.2f}")
        else:
            st.error("Account not found.")

elif choice == "Withdraw":
    st.subheader("Withdraw Money")
    acc_no = st.text_input("Account Number")
    pin = st.text_input("PIN", type="password")
    amount = st.number_input("Amount to Withdraw", min_value=1.0)
    
    if st.button("Withdraw"):
        if acc_no in accounts and accounts[acc_no]["pin"] == pin:
            if accounts[acc_no]["balance"] - amount >= 500:
                accounts[acc_no]["balance"] -= amount
                save_data(accounts)
                st.success(f"Withdrawn successfully! New Balance: Rs. {accounts[acc_no]['balance']:.2f}")
            else:
                st.error("Insufficient balance! Minimum Rs. 500 must be maintained.")
        else:
            st.error("Invalid Account Number or PIN.")

elif choice == "All Accounts":
    st.subheader("All Registered Accounts")
    if accounts:
        for acc_no, details in accounts.items():
            st.write(f"**A/C No:** {acc_no} | **Name:** {details['name']} | **Type:** {details['type']} | **Balance:** Rs. {details['balance']:.2f}")
    else:
    	st.info("No accounts found.")
